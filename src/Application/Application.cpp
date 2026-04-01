#include "Application.h"
#include "Resources.h"
#include "Chess/Board.h"
#include "Graphics/DebugContext.h"
#include "Graphics/Renderer.h"
#include "Graphics/Texture.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

Application* Application::s_Instance = nullptr;

Application::Application(uint32_t width, uint32_t height, const std::string& name)
{
    if (!s_Instance)
        s_Instance = this;

    if (!glfwInit()) {
        std::cout << "Could not initialize GLFW!\n";
        return;
    }

#if _DEBUG
    glfwWindowHint(GLFW_CONTEXT_DEBUG, true);
#endif

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    m_WindowProperties = { (uint32_t)mode->width, (uint32_t)mode->height, name };
    m_ChessViewportSize = { (float)mode->width, (float)mode->height };


    m_Window = glfwCreateWindow(mode->width, mode->height, name.c_str(), NULL, NULL);
    if (!m_Window) {
        std::cout << "Could not create window!\n";
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_Window);

    glfwSwapInterval(1);
    
    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
    {
        Get().OnWindowClose();
    });

    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
	{
    	Get().OnWindowResize(width, height);
    });

    glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        Get().OnKeyPressed(key, scancode, action, mods);
    });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
    {
        Get().OnMouseButton(button, action, mods);
    });

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

#if _DEBUG
    DebugContext::Init();
#endif

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

void Application::Run() {
    m_Running = true;

    Init();

    while (m_Running) {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        RenderImGui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_Window);

        glfwPollEvents();
    }
}

// static glm::vec4 HexToColour(uint32_t colour) {
//     static constexpr float normalise = 1.0f / 255.0f;
//     uint8_t r = (colour & 0xff000000) >> 24;
//     uint8_t g = (colour & 0x00ff0000) >> 16;
//     uint8_t b = (colour & 0x0000ff00) >> 8;
//     uint8_t a = (colour & 0x000000ff);
//     return glm::vec4{ r, g, b, a } * normalise;
// }

std::shared_ptr<SubTexture> Application::GetChessSprite(Piece p) {
    switch (p) {
    case WhitePawn:   return m_ChessPieceSprites[11];
    case WhiteKnight: return m_ChessPieceSprites[9];
    case WhiteBishop: return m_ChessPieceSprites[8];
    case WhiteRook:   return m_ChessPieceSprites[10];
    case WhiteQueen:  return m_ChessPieceSprites[7];
    case WhiteKing:   return m_ChessPieceSprites[6];
    case BlackPawn:   return m_ChessPieceSprites[5];
    case BlackKnight: return m_ChessPieceSprites[3];
    case BlackBishop: return m_ChessPieceSprites[2];
    case BlackRook:   return m_ChessPieceSprites[4];
    case BlackQueen:  return m_ChessPieceSprites[1];
    case BlackKing:   return m_ChessPieceSprites[0];
    case None:        return nullptr;
    }

    throw std::runtime_error("Invalid Piece enum!");
}

void Application::Init() {
    Renderer::Init(glm::ortho(-8.0f, 8.0f, -4.5f, 4.5f), Resources::Shaders::VERTEX_SHADER, Resources::Shaders::FRAGMENT_SHADER);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;

    void* font = (void*)Resources::Fonts::Roboto::ROBOTO_REGULAR;
    int32_t fontSize = sizeof(Resources::Fonts::Roboto::ROBOTO_REGULAR);
    io.FontDefault = io.Fonts->AddFontFromMemoryTTF(font, fontSize, 20.0f, &fontConfig);

    //font = (void*)Resources::Fonts::Roboto::ROBOTO_BOLD;
    //fontSize = sizeof(Resources::Fonts::Roboto::ROBOTO_BOLD);
    //io.Fonts->AddFontFromMemoryTTF(font, fontSize, 20.0f, &fontConfig);

    if (!std::filesystem::exists("imgui.ini"))
        ImGui::LoadIniSettingsFromMemory(Resources::DEFAULT_IMGUI_INI);

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.GrabRounding = 4.0f;
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.WindowMinSize = { 200.0f, 200.0f };

    std::shared_ptr<Texture> chessPieces = std::make_shared<Texture>(Resources::Textures::CHESS_PIECES, sizeof(Resources::Textures::CHESS_PIECES));

    const float tileSize = (float)chessPieces->GetWidth() / 6.0f;
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < 6; x++)
            m_ChessPieceSprites[y * 6 + x] = std::make_shared<SubTexture>(chessPieces, glm::vec2(x, y), glm::vec2(tileSize));

    // m_DarkSquareColour = HexToColour(0x532A00FF);
    // m_LightSquareColour = HexToColour(0xFFB160FF);
    // m_DarkSquareColour = HexToColour(0x90EE90FF);  // Light green
    // m_LightSquareColour = HexToColour(0xFFFFFF);   // White 

    m_DarkSquareColour = glm::vec4(0.37f, 1.0f, 0.37f, 1.0f);  // RGB values from #5FFF5FFF (green)
    m_LightSquareColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);   // White

    m_LegalMoveColour = { 1.0f, 0.0f, 1.0f, 0.5f };
    m_BackgroundColour = { 0.2f, 0.2f, 0.2f, 1.0f };

    m_BoardFEN = Board::START_FEN;
    m_BoardFEN.resize(100);

    FramebufferSpecification spec;
    spec.Width = m_WindowProperties.Width;
    spec.Height = m_WindowProperties.Height;
    m_ChessViewport = std::make_shared<Framebuffer>(spec);
}

void Application::RenderImGui() {
    static bool s_ShowSettingsWindow = true, s_ShowFENWindow = true, s_ShowEngineWindow = true;

    {
        // Fullscreen stuff
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", nullptr, window_flags);
        ImGui::PopStyleVar();

        ImGui::PopStyleVar(2);

        // Submit the DockSpace
        static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiID dockspaceID = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("New");

                ImGui::Separator();

                if (ImGui::MenuItem("Quit"))
                    m_Running = false;

                ImGui::EndMenu();
            }
            else if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Colours")) { s_ShowSettingsWindow = true; }
                if (ImGui::MenuItem("FEN"))     { s_ShowFENWindow     = true; }
                if (ImGui::MenuItem("Engine"))  { s_ShowEngineWindow  = true; }

                ImGui::EndMenu();
            }
            else if (ImGui::BeginMenu("About")) {
                ImGui::Text("OpenGL Version: %s", Renderer::GetOpenGLVersion());

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        

        ImGui::End();
    }

    if (s_ShowFENWindow) {
        ImGui::Begin("FEN", &s_ShowFENWindow);

        if (ImGui::InputText("##FEN", m_BoardFEN.data(), m_BoardFEN.size(), ImGuiInputTextFlags_EnterReturnsTrue))
            m_Board.FromFEN(m_BoardFEN);

        if (ImGui::Button("Copy FEN to clipboard"))
            glfwSetClipboardString(m_Window, m_BoardFEN.c_str());

        if (ImGui::Button("Reset board")) {
            m_Board.Reset();  // Reset FEN string
            m_BoardFEN = Board::START_FEN;
            if (m_RunningEngine)
                m_RunningEngine->SetPosition(m_BoardFEN);
        }

        ImGui::End();
    }


    RenderChessPanel();
    RenderSettingsPanel(&s_ShowSettingsWindow);
    RenderEnginePanel(&s_ShowEngineWindow);


   if (m_ShowPromotionPopup) {
    float sqSize = m_ChessPanelSize.x / 8.0f;
    Colour colour = GetColour(m_Board[m_PromotionFrom]);

    // Position popup based on which color is promoting
    float startY;
    if (colour == White) {
        // White promotes at rank 7 (top of board)
        startY = m_ChessPanelPos.y;
    } else {
        // Black promotes at rank 0 (bottom of board)
        startY = m_ChessPanelPos.y + m_ChessPanelSize.y - sqSize * 4.0f;
    }

    // For White, place the popup on the left; for Black, place it on the right with an additional offset
    float startX;
    float leftBoundary = m_ChessPanelPos.x;
    float rightBoundary = m_ChessPanelPos.x + m_ChessPanelSize.x - sqSize * 1.0f;  // Right boundary for piece selection

    if (colour == White) {
        // Left of the chessboard for White
        startX = m_ChessPanelPos.x - sqSize * 2.0f;
    } else {
        // Right of the chessboard for Black
        startX = m_ChessPanelPos.x + m_ChessPanelSize.x - sqSize * 2.0f + sqSize * 1.0f;  
    }

    startX = std::max(startX, leftBoundary);  // Prevent moving beyond the left boundary
    startX = std::min(startX, rightBoundary); // Prevent moving beyond the right boundary

    // Set the window position and size
    ImGui::SetNextWindowPos({ startX, startY }, ImGuiCond_Always);

    // Set the window size to match the button's size, which is sqSize
    ImGui::SetNextWindowSize({ sqSize * 1.0f, sqSize * 4.0f }, ImGuiCond_Always);  // Adjust the size of the popup window to fit buttons
    ImGui::SetNextWindowBgAlpha(1.0f);

    ImGui::Begin("##promo", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDocking);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.0f, 0.0f });

    constexpr PieceType options[4] = { Queen, Rook, Bishop, Knight };
    constexpr const char* ids[4] = { "##Q", "##R", "##B", "##N" };  // IDS

    for (int i = 0; i < 4; i++) {
        auto sub = GetChessSprite(PieceTypeAndColour(options[i], colour));
        ImTextureID texID = (ImTextureID)(intptr_t)sub->GetTexture()->GetTextureID();
        const glm::vec2* coords = sub->GetTextureCoordinates();

        // Texture is flipped vertically on load, so coords[3]=top-left, coords[1]=bottom-right
        ImVec2 uv0 = { coords[3].x, coords[3].y };
        ImVec2 uv1 = { coords[1].x, coords[1].y };

        if (ImGui::ImageButton(ids[i], texID, { sqSize, sqSize }, uv0, uv1)) {
            m_Board.Move(LongAlgebraicMove{ m_PromotionFrom, m_PromotionTo, options[i] });
            m_BoardFEN = m_Board.ToFEN();
            if (m_RunningEngine)
                m_RunningEngine->SetPosition(m_BoardFEN);
            m_ShowPromotionPopup = false;
            m_SelectedPiece = INVALID_SQUARE;
            m_LegalMoves = 0;
            CheckGameStatus();
            break;
        }
    }

    ImGui::PopStyleVar(2);

    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
        && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_ShowPromotionPopup = false;
        m_SelectedPiece = INVALID_SQUARE;
        m_LegalMoves = 0;
    }

    ImGui::End();
    }

    // Game Over Popup
    if (m_GameOver) {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::OpenPopup("Game Over");

        if (ImGui::BeginPopupModal("Game Over", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::SetWindowFontScale(1.5f);
            ImGui::Text("GAME OVER");
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Spacing();
            ImGui::TextWrapped("%s", m_GameOverReason.c_str());

            ImGui::Spacing();
            ImGui::Separator();

            if (ImGui::Button("New Game", ImVec2(150, 40))) {
                m_Board.Reset();
                m_BoardFEN = Board::START_FEN;
                m_GameOver = false;
                m_GameOverReason.clear();
                m_SelectedPiece = INVALID_SQUARE;
                m_LegalMoves = 0;
                if (m_RunningEngine)
                    m_RunningEngine->SetPosition(m_BoardFEN);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Close", ImVec2(150, 40))) {
                m_GameOver = false;
                m_GameOverReason.clear();
                m_Running = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}

void Application::CheckGameStatus() {
    Colour currentPlayer = m_Board.GetPlayerTurn();
    bool hasLegalMoves = m_Board.HasLegalMoves(currentPlayer);

    if (!hasLegalMoves) {
        // Check if current player is in check by testing if any enemy piece attacks the king
        Square kingSquare = m_Board.findKing(currentPlayer);
        BitBoard controlledByEnemy = m_Board.ControlledSquares(OppositeColour(currentPlayer));
        bool isInCheck = (controlledByEnemy & (1ull << kingSquare)) != 0;

        if (isInCheck) {
            // Checkmate
            Colour winner = OppositeColour(currentPlayer);
            m_GameOverReason = std::string(winner == White ? "White" : "Black") + " wins by checkmate!";
        } else {
            // Stalemate
            m_GameOverReason = "Game drawn by stalemate!";
        }

        m_GameOver = true;
    }
}



void Application::OnWindowClose() {
    m_Running = false;
}

void Application::OnWindowResize(int32_t width, int32_t height) {
    m_WindowProperties.Width = width;
    m_WindowProperties.Height = height;
}

void Application::OnKeyPressed(int32_t key, int32_t scancode, int32_t action, int32_t mods) {
    if (key == GLFW_KEY_ESCAPE) {
        m_Running = false;
    }
}
//Promotion helper function to show the promotion popup and set the promotion squares
bool Application::IsPromotionMove(Square from, Square to) {
    Piece p = m_Board[from];
    if (GetPieceType(p) != Pawn) return false;

    uint8_t destRank = RankOf(to);
    
    // Check for White promotion (Rank 7) and Black promotion (Rank 0)
    return (GetColour(p) == White && destRank == 7) || (GetColour(p) == Black && destRank == 0);
}

void Application::TryMove(Square from, Square to) {
    if (!(m_LegalMoves & (1ull << to))) return;

    if (IsPromotionMove(from, to)) {
        m_PromotionFrom        = from;
        m_PromotionTo          = to;
        m_ShowPromotionPopup   = true;
        // Do NOT call m_Board.Move() yet
    } else {
        m_Board.Move({ from, to });
        m_BoardFEN = m_Board.ToFEN();
        if (m_RunningEngine)
            m_RunningEngine->SetPosition(m_BoardFEN);
        CheckGameStatus();
    }
}

void Application::OnMouseButton(int32_t button, int32_t action, int32_t mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        // Convert ImGui viewport coordinates to rendering coordinates
        glm::vec2& point = m_BoardMousePosition;

        if (action == GLFW_PRESS) {
            m_IsHoldingPiece = true;

            if (point.x > -4 && point.x < 4 && point.y > -4 && point.y < 4) {
                Square rank = (Square)(point.x + 4.0f);
                Square file = (Square)(point.y + 4.0f);

                // The square the mouse clicked on
                Square selectedSquare = ToSquare('a' + rank, '1' + file);

                // If a piece was already selected, move piece to clicked square
                if (m_SelectedPiece != INVALID_SQUARE && m_SelectedPiece != selectedSquare) {
                    if (m_LegalMoves & (1ull << selectedSquare) || selectedSquare == m_SelectedPiece) {
                    
                        TryMove(m_SelectedPiece, selectedSquare);
                    }

                    m_SelectedPiece = INVALID_SQUARE;
                    m_LegalMoves = 0;
                }
                else {  // If no piece already selected, select piece
                    m_LegalMoves = m_Board.GetPieceLegalMoves(selectedSquare);
                    m_SelectedPiece = m_LegalMoves == 0 ? INVALID_SQUARE : selectedSquare;
                }
            }
            else {
                m_SelectedPiece = INVALID_SQUARE;
                m_LegalMoves = 0;
            }
        }
        else if (action == GLFW_RELEASE) {
            if (point.x > -4 && point.x < 4 && point.y > -4 && point.y < 4) {
                Square rank = (Square)(point.x + 4.0f);
                Square file = (Square)(point.y + 4.0f);

                // The square the mouse was released on
                Square selectedSquare = ToSquare('a' + rank, '1' + file);

                if (m_SelectedPiece != INVALID_SQUARE) {
                    if (m_LegalMoves & (1ull << selectedSquare)) {
                        TryMove(m_SelectedPiece, selectedSquare);
                        m_LegalMoves = 0;
                    }
                }
            }

            m_IsHoldingPiece = false;
            m_SelectedPiece = INVALID_SQUARE;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        m_SelectedPiece = INVALID_SQUARE;
        m_IsHoldingPiece = false;
        m_LegalMoves = 0;
    }
}


