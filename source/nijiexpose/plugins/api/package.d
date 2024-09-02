/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.plugins.api;
import lumars;

public import nijiexpose.plugins.api.base;
public import nijiexpose.plugins.api.scene;
public import nijiexpose.plugins.api.ui;

void insPluginRegisterAll(LuaState* state) {
    insRegisterBaseAPI(state);
    insRegisterSceneAPI(state);
    insRegisterUIAPI(state);
}
