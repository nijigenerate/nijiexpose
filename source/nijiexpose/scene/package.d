/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.scene;
import nijilive.core.animation.player;
import nijilive.math.triangle;
import nijilive.core.nodes.utils: removeByValue;
import nijilive;
import nijilive.core.dbg;
import inmath;
import nijiui.input;
import nijiui;
import nijiexpose.tracking;
import nijiexpose.animation;
import nijiexpose.tracking.vspace;
import nijiexpose.panels.tracking : insTrackingPanelRefresh;
import nijiexpose.log;
import nijiexpose.plugins;
import nijiexpose.render.spritebatch;
import bindbc.opengl;
import bindbc.imgui : igIsKeyDown, ImGuiKey;
import std.string: format;
import std.math.operations : isClose;
import std.algorithm: sort, countUntil;
import std.algorithm.iteration: map;
import std.array;
import std.path;

class Scene {
    VirtualSpace space;
    SceneItem[] sceneItems;

    string bgPath;
    Texture backgroundImage;

    bool shouldPostProcess = true;
    float zoneInactiveTimer = 0;

    bool sleeping = false;

    void addPuppet(string path, Puppet puppet) {

        import std.format : format;
        SceneItem item = new SceneItem;
        item.filePath = path;
        item.puppet = puppet;
        item.puppet.root.build();
        item.puppetRoot = puppet.root;
        item.attachedParent = null;
        item.player = new AnimationPlayer(puppet);
        
        if (!item.tryLoadBindings()) {
            // Reset bindings
            item.bindings.length = 0;
        }
        if (!item.tryLoadAnimations()) {
            // Reset animations
            item.animations.length = 0;
        }

        item.genBindings();
        item.genAnimationControls();

        if(this.sleeping) item.sleep();
        this.sceneItems ~= item;
    }

    void init() {
        insScene.space = insLoadVSpace();
        auto tex = ShallowTexture(cast(ubyte[])import("tex/ui-delete.png"));
        inTexPremultiply(tex.data);
        trashcanTexture = new Texture(tex);
        AppBatch = new SpriteBatch();

        insScene.bgPath = inSettingsGet!string("bgPath");
        if (insScene.bgPath) {
            try {
                tex = ShallowTexture(insScene.bgPath);
                if (tex.channels == 4) {
                    inTexPremultiply(tex.data);
                }
                insScene.backgroundImage = new Texture(tex);
            } catch (Exception ex) {
                insLogErr("%s", ex.msg);
            }
        }

        insScene.shouldPostProcess = inSettingsGet!(bool)("shouldPostProcess", true);
        
        float[3] ambientLight = inSettingsGet!(float[3])("ambientLight", [1, 1, 1]);
        inSceneAmbientLight.vector = ambientLight;

        float[4] bgColor = inSettingsGet!(float[4])("bgColor", [0.5, 0.5, 0.5, 0]);
        inSetClearColor(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
    }

    void cleanup() {
        insSaveVSpace(this.space);

        foreach(ref source; this.space.getAllSources()) {
            if (source) {
                if (source.isRunning()) {
                    source.stop();
                }
                destroy(source);
            }
        }
    }

    void update() {
        // Get viewport
        int viewportWidth, viewportHeight;
        inGetViewport(viewportWidth, viewportHeight);

        // Update physics managment
        inUpdate();

        // Update virtual spaces
        this.space.update();

        // Render the waifu trashcan outside of the main FB
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        trashcanVisibility = dampen(trashcanVisibility, isDragDown ? 0.85 : 0, deltaTime(), 1);
        {
            float trashcanScale = 1f;
            float sizeOffset = 0f;


            if (isMouseOverDelete) {
                float scalePercent = (sin(currentTime()*2)+1)/2;
                trashcanScale += 0.15*scalePercent;
                sizeOffset = ((trashcanSize*trashcanScale)-trashcanSize)/2;
            }

            AppBatch.draw(
                trashcanTexture,
                rect(
                    TRASHCAN_DISPLACEMENT-sizeOffset, 
                    viewportHeight-(trashcanSize+TRASHCAN_DISPLACEMENT+sizeOffset),
                    trashcanSize*trashcanScale, 
                    trashcanSize*trashcanScale
                ),
                rect.init,
                vec2(0),
                0,
                SpriteFlip.None,
                vec4(1, 1, 1, trashcanVisibility)
            );
            AppBatch.flush();
            glFlush();
        }
        glDisable(GL_BLEND);

        inBeginScene();

            if (this.backgroundImage) {
                float texWidth = this.backgroundImage.width;
                float texHeight = this.backgroundImage.height;
                
                float scale = max(cast(float)viewportWidth/cast(float)texWidth, cast(float)viewportHeight/cast(float)texHeight);
                
                rect bounds = rect(
                    0,
                    0,
                    texWidth*scale,
                    texHeight*scale
                );

                bounds.x = (viewportWidth/2);
                bounds.y = (viewportHeight/2);
                
                AppBatch.draw(
                    this.backgroundImage,
                    bounds,
                    rect.init,
                    vec2(bounds.width/2, bounds.height/2)
                );
                AppBatch.flush();
            }
            
            // Update plugins
            foreach(ref plugin; insPlugins) {
                if (!plugin.isEnabled) continue;

                if (plugin.hasEvent("onUpdate")) {
                    plugin.callEvent("onUpdate", deltaTime());
                }
            }

            
            if (!this.space.isCurrentZoneActive()) {
                this.zoneInactiveTimer += deltaTime();
                if (this.zoneInactiveTimer >= 5) {
                    if(!this.sleeping){
                        foreach(ref sceneItem; this.sceneItems) {
                            sceneItem.sleep();
                        }
                        this.sleeping = true;
                    }
                }
            } else {
                this.zoneInactiveTimer -= deltaTime();
                // Stop sleep animation
                if (this.sleeping) {
                    foreach(ref sceneItem; this.sceneItems) {
                        sceneItem.awake();
                    }
                    this.sleeping = false;
                }
            }
            this.zoneInactiveTimer = clamp(this.zoneInactiveTimer, 0, 6);

            // Update every scene item
            foreach(ref sceneItem; this.sceneItems) {
                sceneItem.draw();
            }
        inEndScene();

        if (this.shouldPostProcess) {
            inPostProcessScene();
        }

        if (measureFPS) {
            double latestTime = currentTime();
            loopCount ++;
            if (latestTime - lastMeasureTime >= 1.0) {
                lastFPS = loopCount / (latestTime - lastMeasureTime);
                lastMeasureTime = latestTime;
                loopCount = 0;
            }
        }
    }

    /**
        Returns a pointer to the active scene item
    */
    SceneItem selectedSceneItem() {
        if (selectedPuppet < 0 || selectedPuppet >= this.sceneItems.length) return null;
        return this.sceneItems[selectedPuppet];
    }

    void interact() {

        // Skip doing stuff is mouse drag begin in the UI
        if (inInputMouseDownBegannijiui(MouseButton.Left)) return;

        int width, height;
        inGetViewport(width, height);
        
        deleteArea = rect(0, height-(TRASHCAN_DISPLACEMENT+trashcanSize), trashcanSize+TRASHCAN_DISPLACEMENT, trashcanSize+TRASHCAN_DISPLACEMENT);
        isMouseOverDelete = deleteArea.intersects(inInputMousePosition());

        import std.stdio : writeln;
        inCamera = inGetCamera();
        vec2 mousePos = inInputMousePosition();
        vec2 mouseOffset = vec2(width/2, height/2);
        vec2 cameraCenter = inCamera.getCenterOffset();
        mousePos = vec2(
            vec4(
                (mousePos.x-mouseOffset.x+inCamera.position.x)/inCamera.scale.x,
                (mousePos.y-mouseOffset.y+inCamera.position.y)/inCamera.scale.y,
                0, 
                1
            )
        );

        ItemHitTest doHitTestOnItem(Puppet stopTarget) {
            ItemHitTest result;
            foreach(i, sceneItem; insScene.sceneItems) {
                if (sceneItem.puppet == stopTarget)
                    break;
                if (sceneItem.intersects(mousePos)) {
                    result.item = sceneItem;
                    result.index = i;
                    result.item.updateTransform();
                }
            }
            return result;
        }

        if (!inInputWasMouseDown(MouseButton.Left) && inInputMouseDown(MouseButton.Left)) {
            import std.stdio;
            // One shot check if there's a puppet to drag under the cursor
            if (!hasDonePuppetSelect) {
                hasDonePuppetSelect = true;

                // For performance sake we should disable bounds calculation after we're done getting drag state.
                inSetUpdateBounds(true);

                draggingItem = doHitTestOnItem(null);
                if (draggingItem) {
                    selectedPuppet = draggingItem.index;
                    insTrackingPanelRefresh();
                    movingTarget = draggingItem.item;
                    while (movingTarget.attachedParent) {
                        movingTarget = movingTarget.attachedParent;
                        movingTarget.updateTransform();
                    }
                } else {
                    selectedPuppet = -1;
                    insTrackingPanelRefresh();
                    movingTarget = null;
                }
                inSetUpdateBounds(false);
                
            }
        } else if (!inInputMouseDown(MouseButton.Left) && hasDonePuppetSelect) {
            hasDonePuppetSelect = false;
        }

        // Model Scaling
        if (hasDonePuppetSelect && draggingItem) {
            import bindbc.imgui : igSetMouseCursor, ImGuiMouseCursor;
            igSetMouseCursor(ImGuiMouseCursor.Hand);
            float prevScale = draggingItem.item.targetScale;

            float targetDelta = (inInputMouseScrollDelta()*0.05)*(1-clamp(draggingItem.item.targetScale, 0, 0.45));
            draggingItem.item.targetScale = clamp(
                draggingItem.item.targetScale+targetDelta, 
                0.25,
                5
            );
            
            if (draggingItem.item.targetScale != prevScale) {
                inSetUpdateBounds(true);
                    vec4 lbounds = draggingItem.item.puppet.transform.matrix*draggingItem.item.puppet.getCombinedBounds!true();
                    vec2 tl = vec4(lbounds.xy, 0, 1);
                    vec2 br = vec4(lbounds.zw, 0, 1);
                    draggingItem.item.targetSize = abs(br-tl);
                inSetUpdateBounds(false);
            }
        }

        // Model Movement
        if (inInputMouseDragging(MouseButton.Left) && hasDonePuppetSelect && draggingItem) {
            vec2 delta = inInputMouseDragDelta(MouseButton.Left);
            draggingItem.item.targetPos = vec2(
                draggingItem.item.startPos.x+delta.x/inCamera.scale.x, 
                draggingItem.item.startPos.y+delta.y/inCamera.scale.y, 
            );

            if (movingTarget) {
                movingTarget.targetPos = vec2(
                    movingTarget.startPos.x+delta.x/inCamera.scale.x,
                    movingTarget.startPos.y+delta.y/inCamera.scale.y
                );
            }
        }
        
        if (draggingItem) {
            // Model clamping
            float camPosClampX = (cameraCenter.x*2)+(draggingItem.item.targetSize.x/3);
            float camPosClampY = (cameraCenter.y*2)+(draggingItem.item.targetSize.y/1.5);

            // Clamp model to be within viewport
            draggingItem.item.targetPos.x = clamp(
                draggingItem.item.targetPos.x,
                (inCamera.position.x-camPosClampX)*inCamera.scale.x,
                (inCamera.position.x+camPosClampX)*inCamera.scale.x
            );
            draggingItem.item.targetPos.y = clamp(
                draggingItem.item.targetPos.y,
                (inCamera.position.y-camPosClampY)*inCamera.scale.y,
                (inCamera.position.y+camPosClampY)*inCamera.scale.y
            );
            // Apply Movement + Scaling
            if (isMouseOverDelete) {

                // If the mouse was let go
                if (isDragDown && !inInputMouseDown(MouseButton.Left)) {
                    if (selectedPuppet >= 0 && selectedPuppet < insScene.sceneItems.length) {
                        
                        import std.algorithm.mutation : remove;
                        insScene.sceneItems = insScene.sceneItems.remove(selectedPuppet);
                        draggingItem.item = null;
                        selectedPuppet = -1;
                        isDragDown = false;
                        return;
                    }
                }
            } else if (igIsKeyDown(ImGuiKey.LeftCtrl) || igIsKeyDown(ImGuiKey.RightCtrl)) {
                int insertAfter(ref SceneItem[] items, SceneItem previous, SceneItem target) {
                    int insertAfterAux(ref SceneItem[] items, SceneItem previous, SceneItem target, ref int baseIndex, bool rootOnly = false) {
                        int result = -1;
                        foreach (item; items) {
                            if (item.attachedParent !is null && rootOnly)
                                continue;
                            if (item == target)
                                continue;
                            else {
                                item.zSort = baseIndex++;
                                int subResult = insertAfterAux(item.children, previous, target, baseIndex);
                                result = max(subResult, result);
                            }
                            if (item == previous) {
                                target.zSort = baseIndex;
                                result = baseIndex ++;
                                insertAfterAux(target.children, previous, target, baseIndex);
                            }
                        }
                        return result;
                    }
                    int baseIndex = 0;
                    auto result = insertAfterAux(items, previous, target, baseIndex, true);
                    items.sort!((a,b)=>a.zSort < b.zSort);
                    return result;
                }
                // Ctrl + drag should detach attached children
                if (draggingItem.item.attachedParent !is null && isDragDown) { 
                    auto prevParent = draggingItem.item.attachedParent;
                    draggingItem.item.detach();
                    insertAfter(insScene.sceneItems, prevParent, draggingItem.item);
                    movingTarget = draggingItem.item;
                    draggingItem.item.updateTransform();
                }
                if (draggingItem && isDragDown && !inInputMouseDown(MouseButton.Left)) { 
                    // Drop the model
                    ItemHitTest hitTest = doHitTestOnItem(draggingItem.item.puppet);
                    if (hitTest.item) {
                        if (draggingItem.item.attachTo(hitTest.item)) {
                            selectedPuppet = insertAfter(insScene.sceneItems, hitTest.item, draggingItem.item);
                        }
                    }
                } else if (draggingItem && isDragDown) { // Dragging
                    ItemHitTest hitTest = doHitTestOnItem(draggingItem.item.puppet);
                    // Hit Test Action
                }
            }

            isDragDown = inInputMouseDown(MouseButton.Left);

            if (igIsKeyDown(ImGuiKey.LeftCtrl) || igIsKeyDown(ImGuiKey.RightCtrl)) {
                float targetDelta = (inInputMouseScrollDelta()*0.05)*(1-clamp(draggingItem.item.targetScale, 0, 0.45));
                draggingItem.item.targetScale = clamp(
                    draggingItem.item.targetScale+targetDelta, 
                    0.25,
                    5
                );
            }
            

            if (isDragDown && isMouseOverDelete) {
                

                draggingItem.item.puppet.transform.translation = dampen(
                    draggingItem.item.puppet.transform.translation,
                    vec3(
                        (inCamera.position.x+(-cameraCenter.x)+128), 
                        (inCamera.position.y+(cameraCenter.y)-128), 
                        0
                    ),
                    inGetDeltaTime()
                );

                // Dampen & clamp scaling
                draggingItem.item.puppet.transform.scale = dampen(
                    draggingItem.item.puppet.transform.scale,
                    vec2(0.025),
                    inGetDeltaTime()
                );
            } else {
                if (movingTarget !is null) {
                    movingTarget.puppet.transform.translation = dampen(
                        movingTarget.puppet.transform.translation,
                        vec3(movingTarget.targetPos, 0),
                        inGetDeltaTime()
                    );

                    // Dampen & clamp scaling
                    movingTarget.puppet.transform.scale = dampen(
                        movingTarget.puppet.transform.scale,
                        vec2(movingTarget.targetScale),
                        inGetDeltaTime()
                    );
                }
            }
        } else isDragDown = false;
    }
}

private {
    bool measureFPS = false;
    double lastFPS = 0;
    double lastMeasureTime = 0;
    uint  loopCount = 0;


    struct ItemHitTest {
        SceneItem item = null;
        long index = -1;
        T opCast(T: bool)() { return item !is null; }
    }

    ptrdiff_t selectedPuppet = -1;
    ItemHitTest draggingItem;
    bool hasDonePuppetSelect;
    SceneItem movingTarget;

    bool isDragDown = false;
    Camera inCamera;

    enum TRASHCAN_DISPLACEMENT = 16;
    float trashcanVisibility = 0;
    float trashcanSize = 64;
    Texture trashcanTexture;
    rect deleteArea;
    bool isMouseOverDelete;

}

class SceneItem {
    string filePath;
    Puppet puppet;
    Node puppetRoot;
    SceneItem attachedParent;
    TrackingBinding[] bindings;
    AnimationControl[] animations;
    AnimationPlayer player;

    vec2 startPos;
    vec2 targetPos    = vec2(0);
    float targetScale = 0;
    vec2 targetSize   = vec2(0);
    SceneItem[] children;
    float zSort = 0;
    
    string name() {
        return baseName(filePath);
    }

    void saveBindings() {
        puppet.extData["com.inochi2d.inochi-session.bindings"] = cast(ubyte[])serializeToJson(bindings);
        inWriteINPExtensions(puppet, filePath);
    }

    void saveAnimations() {
        puppet.extData["com.inochi2d.inochi-session.animations"] = cast(ubyte[])serializeToJson(animations);
        inWriteINPExtensions(puppet, filePath);
    }

    bool tryLoadBindings() {
        if ("com.inochi2d.inochi-session.bindings" in puppet.extData) {
            auto preBindings = deserialize!(TrackingBinding[])(cast(string)puppet.extData["com.inochi2d.inochi-session.bindings"]);

            // finalize the loading
            bindings = [];
            foreach(ref binding; preBindings) {
                if (binding.finalize(puppet)) {
                    bindings ~= binding;
                }
            }
            return true;
        }
        return false;
    }

    bool tryLoadAnimations() {
        if ("com.inochi2d.inochi-session.animations" in puppet.extData) {
            auto preAnimation = deserialize!(AnimationControl[])(cast(string)puppet.extData["com.inochi2d.inochi-session.animations"]);

            // finalize the loading
            animations = [];
            foreach(ref animation; preAnimation) {
                if (animation.finalize(player)) {
                    animations ~= animation;
                }
            }
            return true;
        }
        return false;
    }

    void genBindings() {
        struct LinkSrcDst {
            Parameter dst;
            int outAxis;
        }
        LinkSrcDst[] srcDst;

        // Note down link targets
        // foreach(param; puppet.parameters) {
        //     foreach(ref ParamLink link; param.links) {
        //         srcDst ~= LinkSrcDst(link.link, cast(int)link.outAxis);
        //     }
        // }

        // Note existing bindings
        foreach(ref binding; bindings) {
            srcDst ~= LinkSrcDst(binding.param, binding.axis);
        }

        bool isParamAxisLinked(Parameter dst, int axis) {
            foreach(ref LinkSrcDst link; srcDst) {
                if (link.dst == dst && axis == link.outAxis) return true;
            }
            return false;
        }
        mforeach: foreach(ref Parameter param; puppet.parameters) {

            // Skip all params affected by physics
            foreach(ref Driver driver; puppet.getDrivers()) 
                if (driver.affectsParameter(param)) continue mforeach;
            

            // Loop over X/Y for parameter
            int imax = param.isVec2 ? 2 : 1;
            for (int i = 0; i < imax; i++) {
                if (isParamAxisLinked(param, i)) continue;
                TrackingBinding binding = new TrackingBinding();
                binding.param = param;
                binding.axis = i;
                binding.type = BindingType.RatioBinding;
                (cast(RatioTrackingBinding)(binding.delegated)).inRange = vec2(0, 1);
                binding.outRangeToDefault();

                // binding name assignment
                if (param.isVec2) binding.name = "%s (%s)".format(param.name, i == 0 ? "X" : "Y");
                else binding.name = param.name;

                bindings ~= binding;
            }
        }
    }

    void genAnimationControls() {
        AnimationControl[string] acs; 
        foreach(ref ac; animations) {
            acs[ac.name] = ac;
        }

        foreach(name, ref anim; puppet.getAnimations()) {
            if(name !in acs){
                AnimationControl ac = new AnimationControl();
                ac.name = name;
                ac.finalize(player);

                animations ~= ac;
            }
        }

    }

    void sleep(){
        foreach(ref animation; animations) {
            animation.sleep();
        }
    }

    void awake(){
        foreach(ref animation; animations) {
            animation.awake();
        }
    }

    vec4 getBounds() {
        auto rootItem = this;
        while (rootItem.attachedParent !is null) rootItem = rootItem.attachedParent;
        if (puppetRoot) {
            mat4 matrix = rootItem.puppet.transform.matrix;
            vec4 bounds = puppetRoot.getCombinedBounds!true();
            vec2 tl = (matrix * vec4(bounds.xy, 0, 1)).xy;
            vec2 br = (matrix * vec4(bounds.zw, 0, 1)).xy;
            return vec4(tl, br);
        }
        return vec4.init;
    }

    bool attachTo(SceneItem parent) {
        
        float getMinZSort(Node node) {
            float minZSort = node.zSort;
            foreach (child; node.children) {
                minZSort = min(minZSort, getMinZSort(child));
            }
            return minZSort;
        }

        auto movingTarget = parent;
        while (movingTarget.attachedParent) movingTarget = movingTarget.attachedParent;
        vec2 relPos = (movingTarget.puppet.transform.matrix.inverse * vec4(this.puppet.transform.translation, 1)).xy;
        vec2 relScale = vec2(this.puppet.transform.scale.x / movingTarget.puppet.transform.scale.x,
                            this.puppet.transform.scale.y / movingTarget.puppet.transform.scale.y);

        if (attachedParent !is null)
            attachedParent.children = attachedParent.children.removeByValue(this);

        auto drawables = parent.puppet.getRootParts().sort!((a, b)=> a.zSort < b.zSort).array;
        foreach (node; drawables) {
            auto d = cast(Drawable)node;
            if (d is null) continue;
            auto posInDrawable = (d.transform.matrix.inverse * vec4(relPos, 0, 1)).xyz;
            vec2 targetScale = vec2(relScale.x / d.transform.scale.x, relScale.y / d.transform.scale.y);
            auto triangle = findSurroundingTriangle(posInDrawable.xy, d.getMesh());
            if (triangle) {
                this.puppetRoot.reparent(node, 0);
                node.transformChanged();
                this.puppetRoot.localTransform.translation = posInDrawable;
                this.puppetRoot.localTransform.scale = targetScale;
                node.transformChanged();
                this.puppetRoot.setAbsZSort(getMinZSort(movingTarget.puppet.root) - 1);
                this.puppetRoot.pinToMesh = true;
                parent.puppet.rescanNodes();
                this.attachedParent = parent;
                parent.children ~= this;
                return true;
            }
        }
        return false;
    }

    bool detach() {
        auto parent = attachedParent;
        auto root = parent;
        while (root.attachedParent) root = root.attachedParent;
        if (root is null) return false;
        import std.stdio;

        Transform curTransform = this.puppetRoot.transform;
        curTransform.translation = (root.puppet.transform.matrix * vec4(curTransform.translation, 1)).xyz;
        curTransform.scale.x *= root.puppet.transform.scale.x;
        curTransform.scale.y *= root.puppet.transform.scale.y;
        curTransform.rotation.z += root.puppet.transform.rotation.z;

        parent.children = parent.children.removeByValue(this);
        this.puppet.setRootNode(this.puppetRoot);
        this.puppet.transform = curTransform;
        this.puppet.transform.update();
        this.puppetRoot.localTransform.translation = vec3(0, 0, 0);
        this.puppetRoot.localTransform.scale       = vec2(1, 1);
        this.puppetRoot.localTransform.rotation    = vec3(0, 0, 0);
        this.puppetRoot.transformChanged();
        this.puppetRoot.zSort = 0;
        this.puppet.rescanNodes();
        this.attachedParent = null;
        this.puppetRoot.pinToMesh = false;
        parent.puppet.rescanNodes();

        void traverse(SceneItem item) {
            item.updateTransform();
            foreach (child; item.children) {
                traverse(child);
            }
        }
        foreach (child; children) {
            traverse(child);
        }
        return true;
    }

    bool intersects(vec2 mousePos) {
        ItemHitTest result;
        // Calculate on-screen bounds of the object
        vec4 lbounds = this.getBounds(); //puppet.getCombinedBounds!true();
        vec2 tl = vec4(lbounds.xy, 0, 1);
        vec2 br = vec4(lbounds.zw, 0, 1);
        vec2 size = abs(br-tl);
        rect bounds = rect(tl.x, tl.y, size.x, size.y);

        if (bounds.intersects(mousePos)) {
            return true;
        }
        return false;
    }

    void draw() {
        foreach(ref binding; this.bindings) {
            binding.update();
        }

        foreach(ref ac; this.animations) {
            ac.update();
        }

        this.player.update(deltaTime());
        this.puppet.update();
        if (this.attachedParent is null) {
            this.puppet.draw();
        }
        /* // Debug
        auto bounds = getBounds();

        float width = bounds.z-bounds.x;
        float height = bounds.w-bounds.y;
        inDbgSetBuffer([
            vec3(bounds.x, bounds.y, 0),
            vec3(bounds.x + width, bounds.y, 0),
            
            vec3(bounds.x + width, bounds.y, 0),
            vec3(bounds.x + width, bounds.y+height, 0),
            
            vec3(bounds.x + width, bounds.y+height, 0),
            vec3(bounds.x, bounds.y+height, 0),
            
            vec3(bounds.x, bounds.y+height, 0),
            vec3(bounds.x, bounds.y, 0),
        ]);
        inDbgLineWidth(3);
        inDbgDrawLines(vec4(.5, .5, .5, 1));
        inDbgLineWidth(1);
        */
        foreach(ref binding; this.bindings) {
            binding.lateUpdate();
        }

    }

    void updateTransform() {
        auto bounds = this.getBounds();
        vec2 size = bounds.zw - bounds.xy;
        this.startPos = this.puppet.transform.translation.xy;
        this.targetScale = this.puppet.transform.scale.x;
        this.targetPos = this.startPos;
        this.targetSize = size;   
    }

    T opCast(T: string)() {
        dump();
    }

    string dump(int index = 0) {
        string result;
        string indent;
        foreach (i; 0..(index*2)) { indent ~= " "; }
        result ~= "%s%s(%.2f)\n".format(indent, name, zSort);
        foreach (child; children) result ~= child.dump(index + 1);
        return result;
    }
}

/**
    List of puppets
*/
Scene singleton;
Scene insScene() {
    if (singleton is null) {
        singleton = new Scene;
    }
    return singleton;
}

bool neGetMeasureFPS() {
    return measureFPS;
}

void neSetMeasureFPS(bool value) {
    measureFPS = value;
}

double neGetFPS() { return lastFPS; }
