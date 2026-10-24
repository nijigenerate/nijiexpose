// Generated from facetrack-d 0.8.0 vtsproto.d.
if (!blendshapes.empty()) { 

                    
                    if (blendshapes.contains("jawOpen")) {

                        if (blendshapes.contains("eyeLookOut_L")) { 
                            blendshapes["ftEyeBlinkLeft"] = blendshapes.at("EyeBlinkLeft");
                            blendshapes["ftEyeXLeft"] = blendshapes.at("eyeLookOut_L")-blendshapes.at("eyeLookIn_L");
                            blendshapes["ftEyeYLeft"] = blendshapes.at("eyeLookUp_L")-blendshapes.at("eyeLookDown_L");
                            blendshapes["ftEyeSquintLeft"] = blendshapes.at("eyeSquint_L");
                            blendshapes["ftEyeWidenLeft"] = blendshapes.at("eyeSquint_L");

                            
                            blendshapes["ftEyeBlinkRight"] = blendshapes.at("EyeBlinkRight");
                            blendshapes["ftEyeXRight"] = blendshapes.at("eyeLookIn_R")-blendshapes.at("eyeLookOut_R");
                            blendshapes["ftEyeYRight"] = blendshapes.at("eyeLookUp_R")-blendshapes.at("eyeLookDown_R");
                            blendshapes["ftEyeSquintRight"] = blendshapes.at("eyeSquint_R");
                            blendshapes["ftEyeWidenRight"] = blendshapes.at("eyeSquint_R");

                            
                            blendshapes["ftMouthOpen"] = blendshapes.at("jawOpen");
                            blendshapes["ftMouthX"] = (1 + blendshapes.at("mouthLeft")-blendshapes.at("mouthRight")) / 2.0;
                            blendshapes["ftMouthEmotion"] = (
                                clamp(
                                    1 +
                                        (blendshapes.at("mouthSmile_L")+blendshapes.at("mouthSmile_R")/2.0) -
                                        (blendshapes.at("mouthFrown_L")+blendshapes.at("mouthFrown_R")/2.0),
                                    0, 2
                                )
                            ) / 2.0;
                        } else if (blendshapes.contains("eyeLookOutLeft")) { 
                            blendshapes["ftEyeBlinkLeft"] = blendshapes.at("eyeBlinkLeft");
                            blendshapes["ftEyeXLeft"] = blendshapes.at("eyeLookOutLeft")-blendshapes.at("eyeLookInLeft");
                            blendshapes["ftEyeYLeft"] = blendshapes.at("eyeLookUpLeft")-blendshapes.at("eyeLookDownLeft");
                            blendshapes["ftEyeSquintLeft"] = blendshapes.at("eyeSquintLeft");
                            blendshapes["ftEyeWidenLeft"] = blendshapes.at("eyeSquintLeft");

                            
                            blendshapes["ftEyeBlinkRight"] = blendshapes.at("eyeBlinkRight");
                            blendshapes["ftEyeXRight"] = blendshapes.at("eyeLookInRight")-blendshapes.at("eyeLookOutRight");
                            blendshapes["ftEyeYRight"] = blendshapes.at("eyeLookUpRight")-blendshapes.at("eyeLookDownRight");
                            blendshapes["ftEyeSquintRight"] = blendshapes.at("eyeSquintRight");
                            blendshapes["ftEyeWidenRight"] = blendshapes.at("eyeSquintRight");

                            
                            blendshapes["ftMouthOpen"] = blendshapes.at("jawOpen");
                            blendshapes["ftMouthX"] = (1 + blendshapes.at("mouthLeft")-blendshapes.at("mouthRight")) / 2.0;
                            blendshapes["ftMouthEmotion"] = (
                                clamp(
                                    1 +
                                        (blendshapes.at("mouthSmileLeft")+blendshapes.at("mouthSmileRight")/2.0) -
                                        (blendshapes.at("mouthFrownLeft")+blendshapes.at("mouthFrownRight")/2.0),
                                    0, 2
                                )
                            ) / 2.0;
                        }

                    } else if (blendshapes.contains("JawOpen")) { 

                        
                        
                        blendshapes["ftEyeBlinkLeft"] = blendshapes.at("EyeBlinkLeft");
                        blendshapes["ftEyeXLeft"] = blendshapes.at("EyeLookOutLeft")-blendshapes.at("EyeLookInLeft");
                        blendshapes["ftEyeYLeft"] = blendshapes.at("EyeLookUpLeft")-blendshapes.at("EyeLookDownLeft");
                        blendshapes["ftEyeSquintLeft"] = blendshapes.at("EyeSquintLeft");
                        blendshapes["ftEyeWidenLeft"] = blendshapes.at("EyeWideLeft");

                        
                        blendshapes["ftEyeBlinkRight"] = blendshapes.at("EyeBlinkRight");
                        blendshapes["ftEyeXRight"] = blendshapes.at("EyeLookInRight")-blendshapes.at("EyeLookOutRight");
                        blendshapes["ftEyeYRight"] = blendshapes.at("EyeLookUpRight")-blendshapes.at("EyeLookDownRight");
                        blendshapes["ftEyeSquintRight"] = blendshapes.at("EyeSquintRight");
                        blendshapes["ftEyeWidenRight"] = blendshapes.at("EyeWideRight");

                        
                        blendshapes["ftMouthOpen"] = clamp(

                            
                            (
                                ((blendshapes.at("MouthLowerDownLeft")+blendshapes.at("MouthUpperUpLeft"))/2) +
                                ((blendshapes.at("MouthLowerDownRight")+blendshapes.at("MouthUpperUpRight"))/2)
                            ),
                            0,
                            1
                        );

                        blendshapes["ftMouthX"] = (1 + blendshapes.at("MouthLeft")-blendshapes.at("MouthRight")) / 2.0;
                        blendshapes["ftMouthEmotion"] = (
                                clamp(
                                    1 +
                                        (blendshapes.at("MouthSmileLeft")+blendshapes.at("MouthSmileRight")/2.0) -
                                        (blendshapes.at("MouthFrownLeft")+blendshapes.at("MouthFrownRight")/2.0),
                                    0, 2
                                )
                            ) / 2.0;
                    }

                    
                    
                }
