#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include "tinyfiledialogs.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <map>
#include <fstream>

namespace fs = std::filesystem;

struct ImageTexture {
    GLuint textureID = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = nullptr;

    ~ImageTexture() {
        if (data) stbi_image_free(data);
        if (textureID) glDeleteTextures(1, &textureID);
    }

    bool loadFromFile(const char* path) {
        if (data) {
            stbi_image_free(data);
            data = nullptr;
        }
        if (textureID) {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }

        data = stbi_load(path, &width, &height, &channels, 0);
        if (!data) return false;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum format = (channels == 4) ? GL_RGBA : (channels == 3) ? GL_RGB : GL_RED;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        return true;
    }
};

struct SpritesheetConfig {
    char inputPath[512] = "";
    char outputDir[256] = "output";
    char spritePrefix[64] = "sprite";
    int spriteWidth = 32;
    int spriteHeight = 32;
    int marginX = 0;
    int marginY = 0;
    int spacingX = 0;
    int spacingY = 0;
    bool showGrid = true;
    float zoomLevel = 1.0f;
    ImVec2 panOffset = ImVec2(0, 0);
};

void setupModernTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.18f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.13f, 0.14f, 0.15f, 0.95f);
    colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.25f, 0.27f, 0.88f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.25f, 0.27f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.35f, 0.37f, 0.40f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.13f, 0.14f, 0.15f, 0.75f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.32f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.53f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.46f, 0.69f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.15f, 0.18f, 0.22f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.20f, 0.26f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 12.0f;

    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 10.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 5.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
}

void Tooltip(const char* desc) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void extractSprite(const unsigned char* imageData, int imageWidth, int imageHeight, int channels,
                   int startX, int startY, int spriteWidth, int spriteHeight,
                   std::vector<unsigned char>& spriteData) {
    spriteData.resize(spriteWidth * spriteHeight * channels);

    for (int y = 0; y < spriteHeight; y++) {
        for (int x = 0; x < spriteWidth; x++) {
            int srcX = startX + x;
            int srcY = startY + y;

            if (srcX >= imageWidth || srcY >= imageHeight) {
                for (int c = 0; c < channels; c++) {
                    spriteData[(y * spriteWidth + x) * channels + c] = 0;
                }
                continue;
            }

            int srcIndex = (srcY * imageWidth + srcX) * channels;
            int dstIndex = (y * spriteWidth + x) * channels;

            for (int c = 0; c < channels; c++) {
                spriteData[dstIndex + c] = imageData[srcIndex + c];
            }
        }
    }
}

int extractSelectedSprites(const SpritesheetConfig& config, const ImageTexture& texture,
                          const std::vector<bool>& selectedSprites,
                          const std::map<int, std::string>& spriteNames,
                          int totalSprites, std::string& statusMsg) {
    if (!texture.data || config.spriteWidth <= 0 || config.spriteHeight <= 0) {
        statusMsg = "Error: Invalid configuration";
        return 0;
    }

    try {
        fs::create_directories(config.outputDir);
    } catch (const std::exception& e) {
        statusMsg = "Error: Failed to create output directory: " + std::string(e.what());
        return 0;
    }

    int availableWidth = texture.width - config.marginX;
    int availableHeight = texture.height - config.marginY;

    int spritesPerRow = std::max(1, (availableWidth + config.spacingX) / (config.spriteWidth + config.spacingX));
    int spritesPerColumn = std::max(1, (availableHeight + config.spacingY) / (config.spriteHeight + config.spacingY));

    int extractedCount = 0;
    std::vector<unsigned char> spriteData;

    for (int row = 0; row < spritesPerColumn; row++) {
        for (int col = 0; col < spritesPerRow; col++) {
            int spriteIndex = row * spritesPerRow + col;

            if (spriteIndex >= totalSprites || !selectedSprites[spriteIndex]) continue;

            int startX = config.marginX + col * (config.spriteWidth + config.spacingX);
            int startY = config.marginY + row * (config.spriteHeight + config.spacingY);

            extractSprite(texture.data, texture.width, texture.height, texture.channels,
                         startX, startY, config.spriteWidth, config.spriteHeight, spriteData);

            std::string filename;
            auto it = spriteNames.find(spriteIndex);
            if (it != spriteNames.end() && !it->second.empty()) {
                filename = it->second + ".png";
            } else {
                filename = std::string(config.spritePrefix) + "_" + std::to_string(spriteIndex) + ".png";
            }

            std::string outputPath = std::string(config.outputDir) + "/" + filename;

            if (stbi_write_png(outputPath.c_str(), config.spriteWidth, config.spriteHeight,
                              texture.channels, spriteData.data(), config.spriteWidth * texture.channels)) {
                extractedCount++;
            }
        }
    }

    statusMsg = "Successfully extracted " + std::to_string(extractedCount) + " sprites!";
    return extractedCount;
}

void drawGrid(ImDrawList* drawList, ImVec2 imagePos, ImVec2 imageSize,
              const ImageTexture& texture, const SpritesheetConfig& config,
              const std::vector<bool>& selectedSprites, int hoveredSprite) {
    if (!config.showGrid || config.spriteWidth <= 0 || config.spriteHeight <= 0) return;

    int availableWidth = texture.width - config.marginX;
    int availableHeight = texture.height - config.marginY;

    int spritesPerRow = std::max(1, (availableWidth + config.spacingX) / (config.spriteWidth + config.spacingX));
    int spritesPerColumn = std::max(1, (availableHeight + config.spacingY) / (config.spriteHeight + config.spacingY));

    float scaleX = imageSize.x / texture.width;
    float scaleY = imageSize.y / texture.height;

    for (int row = 0; row <= spritesPerColumn; row++) {
        float y = config.marginY + row * (config.spriteHeight + config.spacingY);
        if (y > texture.height) break;

        ImVec2 start(imagePos.x, imagePos.y + y * scaleY);
        ImVec2 end(imagePos.x + imageSize.x, imagePos.y + y * scaleY);
        drawList->AddLine(start, end, IM_COL32(66, 150, 250, 200), 2.0f);
    }

    for (int col = 0; col <= spritesPerRow; col++) {
        float x = config.marginX + col * (config.spriteWidth + config.spacingX);
        if (x > texture.width) break;

        ImVec2 start(imagePos.x + x * scaleX, imagePos.y);
        ImVec2 end(imagePos.x + x * scaleX, imagePos.y + imageSize.y);
        drawList->AddLine(start, end, IM_COL32(66, 150, 250, 200), 2.0f);
    }

    for (int row = 0; row < spritesPerColumn; row++) {
        for (int col = 0; col < spritesPerRow; col++) {
            int spriteIndex = row * spritesPerRow + col;
            if (spriteIndex >= (int)selectedSprites.size()) continue;

            float x = config.marginX + col * (config.spriteWidth + config.spacingX);
            float y = config.marginY + row * (config.spriteHeight + config.spacingY);

            ImVec2 rectMin(imagePos.x + x * scaleX, imagePos.y + y * scaleY);
            ImVec2 rectMax(rectMin.x + config.spriteWidth * scaleX, rectMin.y + config.spriteHeight * scaleY);

            if (spriteIndex == hoveredSprite) {
                drawList->AddRectFilled(rectMin, rectMax, IM_COL32(66, 150, 250, 100));
            } else if (selectedSprites[spriteIndex]) {
                drawList->AddRectFilled(rectMin, rectMax, IM_COL32(50, 205, 50, 80));
            }
        }
    }
}

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1600, 900, "Sprite Sheet Slicer", nullptr, nullptr);
    if (window == nullptr) return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 1.5f;

    setupModernTheme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    SpritesheetConfig config;
    ImageTexture spritesheetTexture;
    std::string statusMessage = "Load a spritesheet to begin";
    std::vector<bool> selectedSprites;
    std::map<int, std::string> spriteNames;
    int hoveredSprite = -1;
    int editingSprite = -1;
    char editNameBuffer[64] = "";
    bool selectAll = true;

    ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Sprite Sheet Slicer", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoTitleBar);

        ImGui::BeginChild("Controls", ImVec2(480, 0), true);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
        ImGui::SetWindowFontScale(1.2f);
        ImGui::Text("SPRITE SHEET SLICER");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Input Image");
        ImGui::SetNextItemWidth(-105);
        ImGui::InputText("##FilePath", config.inputPath, sizeof(config.inputPath));
        Tooltip("Path to your spritesheet image file");
        ImGui::SameLine();
        if (ImGui::Button("Browse", ImVec2(95, 0))) {
            const char* filterPatterns[] = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tga"};
            const char* selectedFile = tinyfd_openFileDialog("Select Spritesheet", "", 5, filterPatterns, "Image Files", 0);
            if (selectedFile) {
                strncpy(config.inputPath, selectedFile, sizeof(config.inputPath) - 1);
                config.inputPath[sizeof(config.inputPath) - 1] = '\0';
            }
        }
        Tooltip("Browse for an image file");

        if (ImGui::Button("Load Image", ImVec2(-1, 40))) {
            if (spritesheetTexture.loadFromFile(config.inputPath)) {
                statusMessage = "Image loaded: " + std::to_string(spritesheetTexture.width) + "x" +
                               std::to_string(spritesheetTexture.height) + " pixels";

                int availableWidth = spritesheetTexture.width - config.marginX;
                int availableHeight = spritesheetTexture.height - config.marginY;
                int spritesPerRow = std::max(1, (availableWidth + config.spacingX) / (config.spriteWidth + config.spacingX));
                int spritesPerCol = std::max(1, (availableHeight + config.spacingY) / (config.spriteHeight + config.spacingY));
                int totalSprites = spritesPerRow * spritesPerCol;

                selectedSprites.assign(totalSprites, true);
                spriteNames.clear();
                editingSprite = -1;
                config.zoomLevel = 1.0f;
                config.panOffset = ImVec2(0, 0);
            } else {
                statusMessage = "Error: Failed to load image";
            }
        }
        Tooltip("Load the selected image into the preview");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Sprite Dimensions");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputInt("Width##SpriteW", &config.spriteWidth)) {
            config.spriteWidth = std::max(1, config.spriteWidth);
            if (spritesheetTexture.textureID) {
                int availableWidth = spritesheetTexture.width - config.marginX;
                int availableHeight = spritesheetTexture.height - config.marginY;
                int spritesPerRow = std::max(1, (availableWidth + config.spacingX) / (config.spriteWidth + config.spacingX));
                int spritesPerCol = std::max(1, (availableHeight + config.spacingY) / (config.spriteHeight + config.spacingY));
                int totalSprites = spritesPerRow * spritesPerCol;
                selectedSprites.assign(totalSprites, selectAll);
            }
        }
        Tooltip("Width of each sprite in pixels");

        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputInt("Height##SpriteH", &config.spriteHeight)) {
            config.spriteHeight = std::max(1, config.spriteHeight);
            if (spritesheetTexture.textureID) {
                int availableWidth = spritesheetTexture.width - config.marginX;
                int availableHeight = spritesheetTexture.height - config.marginY;
                int spritesPerRow = std::max(1, (availableWidth + config.spacingX) / (config.spriteWidth + config.spacingX));
                int spritesPerCol = std::max(1, (availableHeight + config.spacingY) / (config.spriteHeight + config.spacingY));
                int totalSprites = spritesPerRow * spritesPerCol;
                selectedSprites.assign(totalSprites, selectAll);
            }
        }
        Tooltip("Height of each sprite in pixels");

        ImGui::Spacing();
        ImGui::Text("Margins");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("Left##MarginX", &config.marginX);
        Tooltip("Offset from the left edge of the image");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("Top##MarginY", &config.marginY);
        Tooltip("Offset from the top edge of the image");

        ImGui::Spacing();
        ImGui::Text("Spacing");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("Horizontal##SpacingX", &config.spacingX);
        Tooltip("Horizontal gap between sprites");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("Vertical##SpacingY", &config.spacingY);
        Tooltip("Vertical gap between sprites");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Output Settings");
        ImGui::SetNextItemWidth(-105);
        ImGui::InputText("##OutputDir", config.outputDir, sizeof(config.outputDir));
        Tooltip("Folder where sprites will be saved");
        ImGui::SameLine();
        if (ImGui::Button("Browse##Dir", ImVec2(95, 0))) {
            const char* selectedFolder = tinyfd_selectFolderDialog("Select Output Directory", config.outputDir);
            if (selectedFolder) {
                strncpy(config.outputDir, selectedFolder, sizeof(config.outputDir) - 1);
                config.outputDir[sizeof(config.outputDir) - 1] = '\0';
            }
        }
        Tooltip("Browse for output folder");

        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("Sprite Prefix", config.spritePrefix, sizeof(config.spritePrefix));
        Tooltip("Default naming prefix (e.g., 'sprite' -> sprite_0.png)");

        ImGui::Spacing();
        ImGui::Checkbox("Show Grid", &config.showGrid);
        Tooltip("Toggle grid overlay visualization");

        if (spritesheetTexture.textureID && !selectedSprites.empty()) {
            int selectedCount = std::count(selectedSprites.begin(), selectedSprites.end(), true);
            ImGui::Text("Selected: %d / %d sprites", selectedCount, (int)selectedSprites.size());

            if (ImGui::Button("Select All", ImVec2(-1, 0))) {
                std::fill(selectedSprites.begin(), selectedSprites.end(), true);
                selectAll = true;
            }
            if (ImGui::Button("Deselect All", ImVec2(-1, 0))) {
                std::fill(selectedSprites.begin(), selectedSprites.end(), false);
                selectAll = false;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Extract Selected Sprites", ImVec2(-1, 50))) {
            if (spritesheetTexture.textureID && !selectedSprites.empty()) {
                extractSelectedSprites(config, spritesheetTexture, selectedSprites,
                                     spriteNames, (int)selectedSprites.size(), statusMessage);
            } else {
                statusMessage = "Error: No image loaded or no sprites selected";
            }
        }
        Tooltip("Export selected sprites to the output folder");

        if (ImGui::Button("Export JSON", ImVec2(-1, 50))) {
            if (spritesheetTexture.textureID && !selectedSprites.empty()) {
                int availableWidth = spritesheetTexture.width - config.marginX;
                int availableHeight = spritesheetTexture.height - config.marginY;
                int spritesPerRow = std::max(1, (availableWidth + config.spacingX) / (config.spriteWidth + config.spacingX));
                int spritesPerCol = std::max(1, (availableHeight + config.spacingY) / (config.spriteHeight + config.spacingY));

                std::string jsonPath = std::string(config.outputDir) + "/spritesheet.json";
                try {
                    fs::create_directories(config.outputDir);
                    std::ofstream jsonFile(jsonPath);
                    if (jsonFile.is_open()) {
                        jsonFile << "{\n";
                        jsonFile << "  \"image\": \"" << config.inputPath << "\",\n";
                        jsonFile << "  \"spriteWidth\": " << config.spriteWidth << ",\n";
                        jsonFile << "  \"spriteHeight\": " << config.spriteHeight << ",\n";
                        jsonFile << "  \"sprites\": [\n";
                        bool first = true;
                        for (int row = 0; row < spritesPerCol; row++) {
                            for (int col = 0; col < spritesPerRow; col++) {
                                int idx = row * spritesPerRow + col;
                                if (idx >= (int)selectedSprites.size() || !selectedSprites[idx]) continue;

                                int x = config.marginX + col * (config.spriteWidth + config.spacingX);
                                int y = config.marginY + row * (config.spriteHeight + config.spacingY);

                                if (!first) jsonFile << ",\n";
                                first = false;

                                std::string name;
                                auto it = spriteNames.find(idx);
                                if (it != spriteNames.end() && !it->second.empty()) {
                                    name = it->second;
                                } else {
                                    name = std::string(config.spritePrefix) + "_" + std::to_string(idx);
                                }

                                jsonFile << "    {\n";
                                jsonFile << "      \"name\": \"" << name << "\",\n";
                                jsonFile << "      \"x\": " << x << ",\n";
                                jsonFile << "      \"y\": " << y << ",\n";
                                jsonFile << "      \"w\": " << config.spriteWidth << ",\n";
                                jsonFile << "      \"h\": " << config.spriteHeight << "\n";
                                jsonFile << "    }";
                            }
                        }
                        jsonFile << "\n  ]\n}\n";
                        jsonFile.close();
                        statusMessage = "JSON exported to " + jsonPath;
                    } else {
                        statusMessage = "Error: Could not write JSON file";
                    }
                } catch (const std::exception& e) {
                    statusMessage = "Error: " + std::string(e.what());
                }
            } else {
                statusMessage = "Error: No image loaded or no sprites selected";
            }
        }
        Tooltip("Export sprite coordinates as JSON for use in your game");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextWrapped("%s", statusMessage.c_str());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text("Program by Miisan");
        ImGui::PopStyleColor();

        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Preview", ImVec2(0, 0), true);

        if (spritesheetTexture.textureID) {
            ImVec2 preview_size = ImGui::GetContentRegionAvail();

            ImGui::Text("Zoom: %.0f%% (Use mouse wheel to zoom)", config.zoomLevel * 100.0f);
            ImGui::SameLine(preview_size.x - 150);
            if (ImGui::Button("Reset View", ImVec2(140, 0))) {
                config.zoomLevel = 1.0f;
                config.panOffset = ImVec2(0, 0);
            }
            Tooltip("Reset zoom and pan");

            preview_size = ImGui::GetContentRegionAvail();
            float aspect = (float)spritesheetTexture.width / (float)spritesheetTexture.height;

            ImVec2 base_image_size;
            if (preview_size.x / aspect < preview_size.y) {
                base_image_size.x = preview_size.x - 20;
                base_image_size.y = base_image_size.x / aspect;
            } else {
                base_image_size.y = preview_size.y - 20;
                base_image_size.x = base_image_size.y * aspect;
            }

            ImVec2 image_size(base_image_size.x * config.zoomLevel, base_image_size.y * config.zoomLevel);

            ImVec2 image_pos = ImGui::GetCursorScreenPos();
            image_pos.x += (preview_size.x - image_size.x) * 0.5f + config.panOffset.x;
            image_pos.y += 10 + config.panOffset.y;

            ImGui::SetCursorScreenPos(image_pos);
            ImGui::Image((void*)(intptr_t)spritesheetTexture.textureID, image_size);

            if (ImGui::IsItemHovered()) {
                float wheel = io.MouseWheel;
                if (wheel != 0) {
                    float zoom_delta = wheel * 0.1f;
                    config.zoomLevel = std::max(0.1f, std::min(5.0f, config.zoomLevel + zoom_delta));
                }

                if (!selectedSprites.empty()) {
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    float relX = (mouse_pos.x - image_pos.x) / image_size.x * spritesheetTexture.width;
                    float relY = (mouse_pos.y - image_pos.y) / image_size.y * spritesheetTexture.height;

                    relX -= config.marginX;
                    relY -= config.marginY;

                    int col = relX / (config.spriteWidth + config.spacingX);
                    int row = relY / (config.spriteHeight + config.spacingY);

                    int availableWidth = spritesheetTexture.width - config.marginX;
                    int availableHeight = spritesheetTexture.height - config.marginY;
                    int spritesPerRow = std::max(1, (availableWidth + config.spacingX) / (config.spriteWidth + config.spacingX));

                    if (col >= 0 && row >= 0 && col < spritesPerRow) {
                        hoveredSprite = row * spritesPerRow + col;
                        if (hoveredSprite >= (int)selectedSprites.size()) hoveredSprite = -1;

                        if (hoveredSprite >= 0 && ImGui::IsMouseClicked(0)) {
                            selectedSprites[hoveredSprite] = !selectedSprites[hoveredSprite];
                        }

                        if (hoveredSprite >= 0 && ImGui::IsMouseClicked(1)) {
                            editingSprite = hoveredSprite;
                            auto it = spriteNames.find(hoveredSprite);
                            if (it != spriteNames.end()) {
                                strncpy(editNameBuffer, it->second.c_str(), sizeof(editNameBuffer) - 1);
                            } else {
                                editNameBuffer[0] = '\0';
                            }
                            ImGui::OpenPopup("EditSpriteName");
                        }
                    } else {
                        hoveredSprite = -1;
                    }
                }
            } else {
                hoveredSprite = -1;
            }

            if (config.showGrid) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawGrid(drawList, image_pos, image_size, spritesheetTexture, config, selectedSprites, hoveredSprite);
            }

            if (ImGui::BeginPopup("EditSpriteName")) {
                ImGui::Text("Edit Sprite Name (Index: %d)", editingSprite);
                ImGui::Separator();
                ImGui::SetNextItemWidth(300);
                ImGui::InputText("##EditName", editNameBuffer, sizeof(editNameBuffer));
                if (ImGui::Button("Save", ImVec2(140, 0))) {
                    if (strlen(editNameBuffer) > 0) {
                        spriteNames[editingSprite] = editNameBuffer;
                    } else {
                        spriteNames.erase(editingSprite);
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(140, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

        } else {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 200);
            ImGui::TextWrapped("Load a spritesheet image to begin.\n\nFeatures:\n- Click sprites to select/deselect\n- Right-click to rename sprites\n- Mouse wheel to zoom in/out\n- Visual grid overlay\n- Custom naming support");
        }

        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
