


// Editor Camera Zoom
// Editor Camera Position
// Editor World View Dimensions in pixels

struct TempLayout
{
    int absx;
    int absy;
    int absw;
    int absh;
};

static Gui::ALH *worldEditorTabLayout = NULL;
static Gui::ALH *worldEntitySelectorLayout = NULL;
static Gui::ALH *worldViewerLayout = NULL;

static void SetupWorldDesigner()
{
    worldEditorTabLayout = Gui::NewALH(false);
    worldEntitySelectorLayout = Gui::NewALH(-1, -1, 180, -1, true);
    worldViewerLayout = Gui::NewALH(true);

    worldEditorTabLayout->Insert(worldEntitySelectorLayout);
    worldEditorTabLayout->Insert(worldViewerLayout);
}

static void WorldDesigner()
{
    editorLayout->Replace(1, worldEditorTabLayout);
    Gui::UpdateMainCanvasALH(editorLayout);


    Gui::PrimitivePanel(Gui::UIRect(worldEntitySelectorLayout), s_EditorColor1);
    Gui::BeginWindow(Gui::UIRect(worldEntitySelectorLayout));

    // Gui::EditorBeginListBox();
    // const std::vector<int>& entityAssetIdsList = *activeEditorState->RetrieveAllEntityAssetIds();
    // for (size_t i = 0; i < entityAssetIdsList.size(); ++i)
    // {
    //     int entityAssetId = entityAssetIdsList.at(i);
    //     EntityAsset *e = activeEditorState->RetrieveEntityAssetById(entityAssetId);
    //     bool selected = entityAssetId == s_SelectedEntityAssetId;
    //     if (Gui::EditorSelectable(e->name.c_str(), &selected))
    //     {
    //         s_SelectedEntityAssetId = entityAssetId;
    //         InitializeCodeEditorState(&s_ActiveCodeEditorState, false, e->code.c_str(), (u32)e->code.size());
    //     }
    // }
    // Gui::EditorEndListBox();
    // Gui::Window_StageLastElementDimension(0, 10);
    Gui::EndWindow();

    // world editor

    TempLayout worldEditorArea;
    worldEditorArea.absx = worldViewerLayout->x;
    worldEditorArea.absy = worldViewerLayout->y;
    worldEditorArea.absw = worldViewerLayout->w;
    worldEditorArea.absh = worldViewerLayout->h;
    if (worldEditorArea.absw % 2 == 1) worldEditorArea.absw += 1; // Note(Kevin): Ortho projection matrix is weird when odd w h t.f. fix to even number
    if (worldEditorArea.absh % 2 == 1) worldEditorArea.absh += 1; // probably do something better in the future cuz now we might render to a canvas 1 pixel larger than expected.

//    worldViewInfo.pan = ivec2(0, 0); // based on mouse panning
//    worldViewInfo.dimAfterZoom = ivec2(worldEditorArea.absw / 1, worldEditorArea.absh / 1);
//    worldViewInfo.dimInUIScale = ivec2(worldEditorArea.absw, worldEditorArea.absh);

    // SpaceAsset& spaceAssetTemp = *activeEditorState->RetrieveSpaceAssetById(activeEditorState->activeSpaceId);

    // if (Input.mouseLeftHasBeenPressed)
    // {
    //     ivec2 clickedInternalCoord = Gfx::GetCoreRenderer()->TransformWindowCoordinateToEditorGUICoordinateSpace(Input.mousePos);
    //     // transform internal coordinate to world viewer coordinate accounting for view zoom pan etc.
    //     ivec2 clickedViewCoord = ivec2(clickedInternalCoord.x - worldEditorArea.absx, clickedInternalCoord.y - worldEditorArea.absy);
    //     if (0 <= clickedViewCoord.x && clickedViewCoord.x < worldEditorArea.absw && 0 <= clickedViewCoord.y && clickedViewCoord.y < worldEditorArea.absh)
    //     {
    //         //printf("hello %d %d \n", clickedViewCoord.x, clickedViewCoord.y);

    //         vec3 thingy225 = vec3(clickedViewCoord.x - (float(worldEditorArea.absw) / 2.f), -clickedViewCoord.y + (float(worldEditorArea.absh) / 2.f), 1.f) / 1.f;

    //         int bruhx = int(floor(thingy225.x)) - worldViewInfo.pan.x;
    //         int bruhy = int(floor(thingy225.y)) - worldViewInfo.pan.y;
    //         printf("thingy225 %d %d \n", bruhx, bruhy);

    //         // vec3 gameCoords = vec3(thingy225.x - worldViewInfo.pan.x, thingy225.y - worldViewInfo.pan.y, 1.f);

    //         // printf("gameCoords %f %f \n", gameCoords.x, gameCoords.y);

    //         EntityAssetInstanceInSpace aardvark;
    //         aardvark.spaceX = bruhx;
    //         aardvark.spaceY = bruhy;
    //         aardvark.entityAssetId = s_SelectedEntityAssetId;
    //         spaceAssetTemp.placedEntities.push_back(aardvark);
    //     }
    // }

    // EntityAssetInstanceInSpace aardvark;
    // aardvark.spaceX = 434;
    // aardvark.spaceY = 0;
    // EntityAssetInstanceInSpace barracuda;
    // barracuda.spaceX = -4;
    // barracuda.spaceY = 4;
    // EntityAssetInstanceInSpace caribou;
    // caribou.spaceX = 32;
    // caribou.spaceY = 0;

    // spaceAssetTemp.placedEntities.push_back(aardvark);
    // spaceAssetTemp.placedEntities.push_back(barracuda);
    // spaceAssetTemp.placedEntities.push_back(caribou);
    //Gfx::BasicFrameBuffer worldViewRender = Gfx::GetCoreRenderer()->RenderTheFuckingWorldEditor(&spaceAssetTemp, activeEditorState, worldViewInfo);

    //Gui::PrimitivePanel(Gui::UIRect(worldEditorArea.absx, worldEditorArea.absy, worldEditorArea.absw, worldEditorArea.absh), worldViewRender.colorTexId);

}

