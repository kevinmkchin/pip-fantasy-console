


// Editor Camera Zoom
// Editor Camera Position
// Editor World View Dimensions in pixels


EditorWorldViewInfo worldViewInfo;
SpaceAsset spaceAssetTemp;

void WorldDesigner()
{
    // Layouts

    Layout worldEditorArea;
    worldEditorArea.absx = 230;
    worldEditorArea.absy = 50;
    worldEditorArea.absw = 450;
    worldEditorArea.absh = 450;


    // DoEntitySelectionPanel();

    worldViewInfo.pan = ivec2(-32, 64); // based on mouse panning
    worldViewInfo.dimAfterZoom = ivec2(225, 225);
    worldViewInfo.dimInUIScale = ivec2(450, 450);

    const int selectionPanelW = EDITOR_FIXED_INTERNAL_RESOLUTION_W/4;
    const int selectionPanelH = EDITOR_FIXED_INTERNAL_RESOLUTION_H/2;

    MesaGUI::PrimitivePanel(MesaGUI::UIRect(0, s_ToolBarHeight, selectionPanelW, selectionPanelH), s_EditorColor1);
    MesaGUI::BeginZone(MesaGUI::UIRect(0, s_ToolBarHeight, selectionPanelW, selectionPanelH));

    EditorState *activeEditorState = EditorState::ActiveEditorState();

    MesaGUI::EditorBeginListBox();
    const std::vector<int> entityAssetIdsList = *activeEditorState->RetrieveAllEntityAssetIds();
    for (size_t i = 0; i < entityAssetIdsList.size(); ++i)
    {
        int entityAssetId = entityAssetIdsList.at(i);
        EntityAsset *e = activeEditorState->RetrieveEntityAssetById(entityAssetId);
        bool selected = entityAssetId == s_SelectedEntityAssetId;
        if (MesaGUI::EditorSelectable(e->name.c_str(), &selected))
        {
            s_SelectedEntityAssetId = entityAssetId;
            InitializeCodeEditorState(&s_ActiveCodeEditorState, false, e->code.c_str(), (u32)e->code.size());
        }
    }
    MesaGUI::EditorEndListBox();

    MesaGUI::MoveXYInZone(0, 10);

    MesaGUI::EndZone();

    ivec2 clickedInternalCoord = Gfx::GetCoreRenderer()->TransformWindowCoordinateToInternalCoordinate(Input.mousePos);
    // transform internal coordinate to world viewer coordinate accounting for view zoom pan etc.

    if (Input.mouseLeftHasBeenPressed) printf("hello %d %d \n", clickedInternalCoord.x, clickedInternalCoord.y);



    // EntityAssetInstanceInSpace aardvark;
    // aardvark.spaceX = 0;
    // aardvark.spaceY = 0;
    // EntityAssetInstanceInSpace barracuda;
    // barracuda.spaceX = 0;
    // barracuda.spaceY = -64;
    // EntityAssetInstanceInSpace caribou;
    // caribou.spaceX = 32;
    // caribou.spaceY = 0;

    // spaceAssetTemp.placedEntities.push_back(aardvark);
    // spaceAssetTemp.placedEntities.push_back(barracuda);
    // spaceAssetTemp.placedEntities.push_back(caribou);
    Gfx::BasicFrameBuffer worldViewRender = Gfx::GetCoreRenderer()->RenderTheFuckingWorldEditor(&spaceAssetTemp, activeEditorState, worldViewInfo);

    MesaGUI::PrimitivePanel(MesaGUI::UIRect(worldEditorArea.absx, worldEditorArea.absy, worldEditorArea.absw, worldEditorArea.absh), worldViewRender.colorTexId);

}

