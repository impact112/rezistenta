#include "Editor.hpp"
#include "ReziSolver.hpp"
#include "gui/Utils.hpp"

Editor *Editor::instance = nullptr;

Editor::Editor(int width, int height, bool fullscreen, std::string title, ReziContext &context) : width(width), height(height), fullscreen(fullscreen), title(title), context(context) {
}

Editor &Editor::GetInstance(int width, int height, int fullscreen, std::string title, ReziContext &context) {
  if (!instance) {
    instance = new Editor(width, height, fullscreen, title, context);
  }
  return *instance;
}

Editor &Editor::GetInstance(void) {
  if (!instance)
    throw std::runtime_error("Instance not created!");
  return *instance;
}

Editor::~Editor(void) {
  UnloadFont(font);
  CloseWindow();
}

void RenderGrid(float interval) {
  rlPushMatrix();
  rlTranslatef(0.0f, 100.0f, 0.0f);
  rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
  DrawGrid(1000 / interval, interval);
  rlPopMatrix();
}

void Editor::Start(void) {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(width, height, title.c_str());
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetExitKey(0);
  Awake();
  if (fullscreen)
    ToggleFullscreen();
#ifndef PLATFORM_WEB
  while (!WindowShouldClose()) {
    Loop();
  }
#endif
}

void Editor::Loop(void) {
  Update();
  BeginDrawing();
  rlDisableBackfaceCulling();
  ClearBackground(BLACK);
  Render();
  RenderTopBar();
  RenderGUI();
  EndDrawing();
}

void Editor::Awake(void) {
  font = GetFontDefault();
  camera = {
      .offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f},
      .target = {0.0f, 0.0f},
      .rotation = 180.0f,
      .zoom = 100.0f};

  saveButton.font = &font;
  saveButton.label = "Save";
  saveButton.size = {50.0f, 20.0f};
  saveButton.position = {0.0f, 0.0f};
  saveButton.CalculateTextPadding();

  loadButton.font = &font;
  loadButton.label = "Load";
  loadButton.size = {50.0f, 20.0f};
  loadButton.position = {saveButton.size.x(), 0.0f};
  loadButton.CalculateTextPadding();

  lockYButton.font = &font;
  lockYButton.label = "Lock Y";
  lockYButton.size = {60.0f, 20.0f};
#ifdef PLATFORM_WEB
  lockYButton.position = {0.0f, 0.0f};
#else
  lockYButton.position = {loadButton.size.x() + loadButton.position.x(), 0.0f};
#endif
  lockYButton.CalculateTextPadding();

  helpButton.font = &font;
  helpButton.label = "Help";
  helpButton.size = {50.0f, 20.0f};
  helpButton.position = {lockYButton.size.x() + lockYButton.position.x(), 0.0f};
  helpButton.CalculateTextPadding();

  nodeTypeButton.font = &font;
  nodeTypeButton.size = {100.0f, 18.0f};
  nodeTypeButton.normalColor = WHITE;
  nodeTypeButton.CalculateTextPadding();

  addNodeButton.font = &font;
  addNodeButton.position = {5.0f, 25.0f};
  addNodeButton.size = {190.0f, 20.0f};
  addNodeButton.label = "Add node";
  addNodeButton.CalculateTextPadding();

  connectButton.font = &font;
  connectButton.position = {5.0f, 50.0f};
  connectButton.size = {190.0f, 20.0f};
  connectButton.label = "Connect nodes";
  connectButton.CalculateTextPadding();

  disconnectButton.font = &font;
  disconnectButton.position = {5.0f, 75.0f};
  disconnectButton.size = {190.0f, 20.0f};
  disconnectButton.label = "Disconnect nodes";
  disconnectButton.CalculateTextPadding();

  coordsBoxX.font = &font;
  coordsBoxX.fontSize = 16.0f;
  coordsBoxX.size = {100.0f, 18.0f};

  coordsBoxY.font = &font;
  coordsBoxY.fontSize = 16.0f;
  coordsBoxY.size = {100.0f, 18.0f};

  forceBoxX.font = &font;
  forceBoxX.fontSize = 16.0f;
  forceBoxX.size = {100.0f, 18.0f};

  forceBoxY.font = &font;
  forceBoxY.fontSize = 16.0f;
  forceBoxY.size = {100.0f, 18.0f};

  momentBox.font = &font;
  momentBox.fontSize = 16.0f;
  momentBox.size = {100.0f, 18.0f};

  nodeRadius = 6.0f;
  forceViewScale = 0.5f;
  momentViewScale = 0.1f;

  lockY = false;
  editorMode = MODE_FREE;
  solver = 't';
  solved = false;
}

std::vector<size_t> Editor::GetSelectedNodeIndexes(void) {
  std::vector<size_t> vec;
  for (size_t i = 0; i < selectedNodes.size(); i++)
    if (selectedNodes.at(i))
      vec.push_back(i);
  return vec;
}

void Editor::Update(void) {
  std::vector<size_t> selNodes = GetSelectedNodeIndexes();
  guiHeight = 120.0f;

  if (IsKeyPressed(KEY_N) || addNodeButton.IsClicked(MOUSE_BUTTON_LEFT))
    editorMode = EditorMode::MODE_ADD;

  if (IsKeyPressed(KEY_ESCAPE)) {
    selectedNodes.assign(selectedNodes.size(), false);
    editorMode = EditorMode::MODE_FREE;
  }

  if (IsKeyPressed(KEY_T)) {
    errMsg.clear();
    solved = false;
    ReziSolver::SolveT(context, errMsg);
    if (errMsg.empty()) {
      statusMsg = "Solved system using T solver.";
      solved = true;
    } else {
      solved = false;
    }
  }

  if (IsKeyPressed(KEY_S) && solver == 't') {
    context.SortX();
  }

  if (lockYButton.IsClicked(MOUSE_BUTTON_LEFT)) {
    if (lockY) {
      lockY = false;
      lockYButton.textColor = LIME;
    } else {
      lockY = true;
      lockYButton.textColor = BLACK;
    }
  }

  if (helpButton.IsClicked(MOUSE_BUTTON_LEFT)) {
    if (editorMode != MODE_INFO)
      editorMode = MODE_INFO;
    else
      editorMode = MODE_FREE;
  }

  if (loadButton.IsClicked(MOUSE_BUTTON_LEFT)) {
    errMsg.clear();
    context.LoadToml("out.toml", errMsg);
    selectedNodes.clear();
    selectedNodes.assign(context.GetNodeCount(), false);
    statusMsg = "Loaded out.toml.";
  }

  if (GetMouseY() > guiHeight && editorMode != MODE_INFO) {
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      Vec2D delta = (Vec2D)GetMouseDelta();
      delta *= -1.0f / camera.zoom;
      camera.target = Vector2Subtract(camera.target, delta);
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      Vec2D mouseWorldPos = (Vec2D)GetScreenToWorld2D({(float)GetMouseX(), (float)GetMouseY()}, camera);
      camera.offset = GetMousePosition();
      camera.target = mouseWorldPos;
      const float zoomIncrement = 1.0f;
      camera.zoom += (wheel * zoomIncrement);
      if (camera.zoom < 25.0f)
        camera.zoom = 25.0f;
      if (camera.zoom < zoomIncrement)
        camera.zoom = zoomIncrement;
    }
  }
  switch (editorMode) {
  case MODE_FREE:
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && GetMouseY() > guiHeight) {
      int hoveredIndex = GetHoveredNode(nodeRadius);
      if (IsKeyDown(KEY_LEFT_SHIFT)) {
        if (hoveredIndex != -1) {
          if (selectedNodes.at(hoveredIndex))
            selectedNodes.at(hoveredIndex) = false;
          else
            selectedNodes.at(hoveredIndex) = true;
        }
      } else if (selNodes.size() == 0 && hoveredIndex != -1) {
        if (!selectedNodes.at(hoveredIndex))
          selectedNodes.at(hoveredIndex) = true;
      } else {
        selectedNodes.assign(selectedNodes.size(), false);
      }
    }
    if (selNodes.size() == 2) {
      if (IsKeyPressed(KEY_C) || connectButton.IsClicked(MOUSE_BUTTON_LEFT)) {
        context.Connect(selNodes.at(0), selNodes.at(1));
        selectedNodes.assign(selectedNodes.size(), false);
      }
      if (IsKeyPressed(KEY_D) || disconnectButton.IsClicked(MOUSE_BUTTON_LEFT)) {
        context.Disconnect(selNodes.at(0), selNodes.at(1));
        selectedNodes.assign(selectedNodes.size(), false);
      }
    } else if (selNodes.size() == 1) {
      Node &node = context.Nodes.at(selNodes.at(0));
      coordsBoxX.target = &node.position.x();
      coordsBoxY.target = &node.position.y();
      forceBoxX.target = &node.cForce.x();
      forceBoxY.target = &node.cForce.y();
      momentBox.target = &node.cMoment;
      nodeTypeButton.label = getNodeTypeName(node.type);
      coordsBoxX.Update();
      coordsBoxY.Update();
      forceBoxX.Update();
      forceBoxY.Update();
      momentBox.Update();
      solved = false;
      if (nodeTypeButton.IsClicked(MOUSE_BUTTON_LEFT)) {
        switch (node.type) {
        case NODE_INVALID:
          node.type = NODE_FREE;
          break;
        case NODE_FREE:
          node.type = NODE_JOINT;
          break;
        case NODE_JOINT:
          node.type = NODE_ARTICULATION;
          break;
        case NODE_ARTICULATION:
          node.type = NODE_BEARING;
          break;
        case NODE_BEARING:
          node.type = NODE_FREE;
          break;
        case NODE_VIRTUAL:
          break;
        }
      }
    }
    if (selNodes.size()) {
      if (IsKeyPressed(KEY_DELETE)) {
        size_t offset = 0;
        for (size_t i : selNodes)
          context.DeleteNode(i - offset++);
        selectedNodes.assign(selectedNodes.size(), false);
      }
    }
    break;
  case MODE_ADD:
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && GetHoveredNode(nodeRadius) == -1 && GetMouseY() > guiHeight) {
      context.AddNode({.type = NODE_FREE,
                       .position = Vec2D(GetScreenToWorld2D(GetMousePosition(), camera).x, (!lockY) ? GetScreenToWorld2D(GetMousePosition(), camera).y : 0.0f),
                       .cForce = {0.0f, 0.0f},
                       .cMoment = 0.0f,
                       .rForce = {0.0f, 0.0f},
                       .rMoment = 0.0f});
      selectedNodes.assign(context.GetNodeCount(), false);
    }
    break;
  case MODE_INFO:
    if (GetMouseY() > guiHeight && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
      editorMode = MODE_FREE;
    break;
  }
}

void Editor::Render(void) {
  ClearBackground(WHITE);
  BeginMode2D(camera);
  RenderGrid(1.0f);
  if (solved && solver == 't') {
    RenderDiagramT(context, -5.0f, 1.0f);
    // RenderDiagramM(context, -10.0f, 1.0f);
  }
  for (size_t i = 0; i < context.GetNodeCount(); i++) {
    Node &node = context.Nodes.at(i);
    for (size_t j = i + 1; j < context.GetNodeCount(); j++)
      if (context.Connections.at(i).at(j))
        DrawLineEx(context.Nodes.at(i).position, context.Nodes.at(j).position, 3.0f / camera.zoom, BLACK);
    DrawNode(node, camera.zoom / 1.5f, nodeRadius / 2.0f, BLACK);
    DrawVector(node.cForce, node.position, forceViewScale, 2.0f / camera.zoom, BLUE);
    DrawMoment(node.cMoment, node.position, momentViewScale, 2.0f / camera.zoom, BLUE);
    if (solved && solver == 't') {
      DrawVector(node.rForce, node.position, forceViewScale, 2.0f / camera.zoom, PURPLE);
      DrawMoment(node.rMoment, node.position, momentViewScale, 2.0f / camera.zoom, PURPLE);
    }
    DrawCircleV(node.position, nodeRadius / camera.zoom, IsNodeHovered(i, nodeRadius) ? GRAY : (node.type == NodeType::NODE_INVALID) ? RED
                                                                                                                                     : BLACK);
  }
  for (size_t i = 0; i < context.GetNodeCount(); i++) {
    if (selectedNodes.at(i))
      DrawCircleV(context.Nodes.at(i).position, nodeRadius / camera.zoom, IsNodeHovered(i, nodeRadius) ? GREEN : LIME);
  }
  EndMode2D();
  if (editorMode == MODE_ADD && GetHoveredNode(nodeRadius) == -1 && GetMouseY() > guiHeight) {
    DrawCircleV(GetMousePosition(), nodeRadius, BLACK);
  }
  for (size_t i = 0; i < context.GetNodeCount(); i++) {

    Node &node = context.Nodes.at(i);
    DrawTextEx(font, TextFormat("(%lu)", i + 1), (Vector2){GetWorldToScreen2D(node.position, camera).x + 20.0f, GetWorldToScreen2D(node.position, camera).y + 30.0f}, 16.0f, 2.0f, BLACK);
  }
  if (solved) {
    switch (solver) {
    case 't':
      for (size_t i = 0; i < context.GetNodeCount(); i++) {
        Node &node = context.Nodes.at(i);
        if (node.type == NODE_FREE || node.type == NODE_INVALID)
          continue;
        DrawTextEx(font, TextFormat("V%lu = %.4f", i + 1, node.rForce.y()), (Vector2){GetWorldToScreen2D(node.position, camera).x + 20.0f, GetWorldToScreen2D(node.position, camera).y + 70.0f}, 16.0f, 2.0f, PURPLE);
      }
      break;
    default:
      break;
    }
  }
  for (Node &node : context.Nodes) {
    if (node.cForce.y() != 0.0f)
      DrawTextEx(font, TextFormat("%.4fF", node.cForce.y()), (Vector2){GetWorldToScreen2D(node.position, camera).x + 5.0f, GetWorldToScreen2D(node.position, camera).y - 40.0f}, 16.0f, 2.0f, BLUE);
    if (node.cMoment != 0.0f)
      DrawTextEx(font, TextFormat("%.4fFl", node.cMoment), (Vector2){GetWorldToScreen2D(node.position, camera).x + 5.0f, GetWorldToScreen2D(node.position, camera).y - 60.0f}, 16.0f, 2.0f, BLUE);
  }
}

bool Editor::IsNodeHovered(size_t index, float radius) {
  if (lockY)
    return abs(GetWorldToScreen2D(context.Nodes.at(index).position, camera).x - GetMousePosition().x) < radius;
  return ((Vec2D)GetWorldToScreen2D(context.Nodes.at(index).position, camera) - (Vec2D)GetMousePosition()).norm() < radius;
}

int Editor::GetHoveredNode(float radius) {
  for (size_t i = 0; i < context.Nodes.size(); i++)
    if (IsNodeHovered(i, radius))
      return i;
  return -1;
}

void Editor::RenderGUI(void) {
  if (errMsg.length()) {
    DrawTextEx(font, errMsg.c_str(), {5.0f, GetScreenHeight() - 21.0f}, 16.0f, 2.0f, RED);
  } else if (statusMsg.length()) {
    DrawTextEx(font, statusMsg.c_str(), {5.0f, GetScreenHeight() - 21.0f}, 16.0f, 2.0f, GREEN);
  }
  DrawRectangleV({0.0f, 20.0f}, {(float)GetScreenWidth(), 100.0f}, LIGHTGRAY);
  addNodeButton.Render();
  if (GetSelectedNodeIndexes().size() == 1) {
    RenderNodeProps();
  } else if (GetSelectedNodeIndexes().size() == 2) {
    connectButton.Render();
    disconnectButton.Render();
  }
  if (editorMode == MODE_INFO) {
    DrawRectangleV({0.0f, guiHeight}, {(float)GetScreenWidth(), (float)GetScreenHeight()}, {0, 0, 0, 64});
    DrawTextEx(font, EditorControls.c_str(), {5.0f, guiHeight + 5.0f}, 16.0f, 2.0f, BLACK);
  } else {
    RenderDebugInfo();
  }
}

void Editor::RenderTopBar(void) {
  DrawRectangleV({0.0f, 0.0f}, {(float)GetScreenWidth(), 20.0f}, GRAY);
#ifndef PLATFORM_WEB
  saveButton.Render();
  loadButton.Render();
#endif
  lockYButton.Render();
  helpButton.Render();
}

void Editor::RenderNodeProps(void) {
  Vec2D offset = {std::max((float)GetScreenWidth() / 3.0f, 200.0f), 20.0f};
  DrawRectangleV(offset, {(float)GetScreenWidth(), 100.0f}, GRAY);
  DrawTextEx(font, TextFormat("Node (%lu) properties:", GetSelectedNodeIndexes().at(0) + 1), {offset.x() + 5.0f, offset.y() + 5.0f}, 16.0f, 2.0f, BLACK);
  DrawTextEx(font, "Type:", {offset.x() + 5.0f, offset.y() + 21.0f}, 16.0f, 2.0f, BLACK);
  DrawTextEx(font, "X:", {offset.x() + 5.0f, offset.y() + 42.0f}, 16.0f, 2.0f, BLACK);
  DrawTextEx(font, "Y:", {offset.x() + 5.0f, offset.y() + 63.0f}, 16.0f, 2.0f, BLACK);
  DrawTextEx(font, "CForce X:", {offset.x() + 200.0f, offset.y() + 21.0f}, 16.0f, 2.0f, BLACK);
  DrawTextEx(font, "CForce Y:", {offset.x() + 200.0f, offset.y() + 42.0f}, 16.0f, 2.0f, BLACK);
  DrawTextEx(font, " CMoment:", {offset.x() + 200.0f, offset.y() + 63.0f}, 16.0f, 2.0f, BLACK);
  nodeTypeButton.position = {offset.x() + 60.0f, offset.y() + 21.0f};
  coordsBoxX.position = {offset.x() + 60.0f, offset.y() + 42.0f};
  coordsBoxY.position = {offset.x() + 60.0f, offset.y() + 63.0f};
  forceBoxX.position = {offset.x() + 300.0f, offset.y() + 21.0f};
  forceBoxY.position = {offset.x() + 300.0f, offset.y() + 42.0f};
  momentBox.position = {offset.x() + 300.0f, offset.y() + 63.0f};
  nodeTypeButton.CalculateTextPadding();
  nodeTypeButton.Render();
  coordsBoxX.Render();
  coordsBoxY.Render();
  forceBoxX.Render();
  forceBoxY.Render();
  momentBox.Render();
}

void Editor::RenderDebugInfo(void) {
  if (GetHoveredNode(nodeRadius) != -1)
    DrawTextEx(font,
               TextFormat("FPS: %d\nNodeCount: %d\nHovered Node: %d\nEditor Mode: %d\nCamera: x:%.2f y:%.2f zoom:%.2f\nMouse: x:%.2f y:%.2f", GetFPS(), context.GetNodeCount(), GetHoveredNode(nodeRadius) + 1, editorMode, camera.target.x, camera.target.y, camera.zoom, GetScreenToWorld2D(GetMousePosition(), camera).x, GetScreenToWorld2D(GetMousePosition(), camera).y),
               {4.0f, guiHeight + 5.0f}, 16.0f, 2.0f, LIME);
  else
    DrawTextEx(font,
               TextFormat("FPS: %d\nNodeCount: %d\nHovered Node: none\nEditor Mode: %d\nCamera: x:%.2f y:%.2f zoom:%.2f\nMouse: x:%.2f y:%.2f", GetFPS(), context.GetNodeCount(), editorMode, camera.target.x, camera.target.y, camera.zoom, GetScreenToWorld2D(GetMousePosition(), camera).x, GetScreenToWorld2D(GetMousePosition(), camera).y),
               {4.0f, guiHeight + 5.0f}, 16.0f, 2.0f, LIME);
}
