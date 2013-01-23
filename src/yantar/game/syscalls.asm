code

equ	trap_Print                            -1
equ	trap_Error                            -2
equ	trap_Milliseconds                     -3
equ	trap_cvarregister                    -4
equ	trap_cvarupdate                      -5
equ	trap_cvarsetstr                         -6
equ	trap_cvargeti        -7
equ	trap_cvargetstrbuf        -8
equ	trap_Argc                             -9
equ	trap_Argv                             -10
equ	trap_FS_FOpenFile                     -11
equ	trap_FS_Read                          -12
equ	trap_FS_Write                         -13
equ	trap_FS_FCloseFile                    -14
equ	trap_SendConsoleCommand               -15
equ	trap_LocateGameData                   -16
equ	trap_DropClient                       -17
equ	trap_SendServerCommand                -18
equ	trap_SetConfigstring                  -19
equ	trap_GetConfigstring                  -20
equ	trap_GetUserinfo                      -21
equ	trap_SetUserinfo                      -22
equ	trap_GetServerinfo                    -23
equ	trap_SetBrushModel                    -24
equ	trap_Trace                            -25
equ	trap_PointContents                    -26
equ	trap_InPVS                            -27
equ	trap_InPVSIgnorePortals               -28
equ	trap_AdjustAreaPortalState            -29
equ	trap_AreasConnected                   -30
equ	trap_LinkEntity                       -31
equ	trap_UnlinkEntity                     -32
equ	trap_EntitiesInBox                    -33
equ	trap_EntityContact                    -34
equ	trap_BotAllocateClient                -35
equ	trap_BotFreeClient                    -36
equ	trap_GetUsercmd                       -37
equ	trap_GetEntityToken                   -38
equ	trap_FS_GetFileList                   -39
equ	trap_DebugPolygonCreate               -40
equ	trap_DebugPolygonDelete               -41
equ	trap_RealTime                         -42
equ	trap_snapv3                           -43
equ	trap_TraceCapsule                     -44
equ	trap_EntityContactCapsule             -45
equ	trap_FS_Seek                          -46

equ	memset                                -47
equ	memcpy                                -48
equ	strncpy                               -49
equ	sin                                   -50
equ	cos                                   -51
equ	atan2                                 -52
equ	sqrt                                  -53
equ	floor                                 -54
equ	ceil                                  -55
equ	acos                                  -56
equ	asin                                  -57
equ	atan                                  -58

equ	testPrintInt                          -59
equ	testPrintFloat                        -60

equ	trap_BotLibSetup                      -61
equ	trap_BotLibShutdown                   -62
equ	trap_BotLibVarSet                     -63
equ	trap_BotLibVarGet                     -64
equ	trap_BotLibDefine                     -65
equ	trap_BotLibStartFrame                 -66
equ	trap_BotLibLoadMap                    -67
equ	trap_BotLibUpdateEntity               -68
equ	trap_BotLibTest                       -69

equ	trap_BotGetSnapshotEntity             -70
equ	trap_BotGetServerCommand              -71
equ	trap_BotUserCommand                   -72



equ	trap_AAS_EnableRoutingArea            -73
equ	trap_AAS_BBoxAreas                    -74
equ	trap_AAS_AreaInfo                     -75
equ	trap_AAS_EntityInfo                   -76

equ	trap_AAS_Initialized                  -77
equ	trap_AAS_PresenceTypeBoundingBox      -78
equ	trap_AAS_Time                         -79

equ	trap_AAS_PointAreaNum                 -80
equ	trap_AAS_TraceAreas                   -81

equ	trap_AAS_PointContents                -82
equ	trap_AAS_NextBSPEntity                -83
equ	trap_AAS_ValueForBSPEpairKey          -84
equ	trap_AAS_VectorForBSPEpairKey         -85
equ	trap_AAS_FloatForBSPEpairKey          -86
equ	trap_AAS_IntForBSPEpairKey            -87

equ	trap_AAS_AreaReachability             -88

equ	trap_AAS_AreaTravelTimeToGoalArea     -89

equ	trap_AAS_Swimming                     -90
equ	trap_AAS_PredictClientMovement        -91



equ	trap_EA_Say                           -92
equ	trap_EA_SayTeam                       -93
equ	trap_EA_Command                       -94

equ	trap_EA_Action                        -95
equ	trap_EA_Gesture                       -96
equ	trap_EA_Talk                          -97
equ	trap_EA_Attack                        -98
equ	trap_EA_Use                           -99
equ	trap_EA_Respawn                       -100
equ	trap_EA_Crouch                        -101
equ	trap_EA_MoveUp                        -102
equ	trap_EA_MoveDown                      -103
equ	trap_EA_MoveForward                   -104
equ	trap_EA_MoveBack                      -105
equ	trap_EA_MoveLeft                      -106
equ	trap_EA_MoveRight                     -107

equ	trap_EA_SelectWeapon                  -108
equ	trap_EA_Jump                          -109
equ	trap_EA_DelayedJump                   -110
equ	trap_EA_Move                          -111
equ	trap_EA_View                          -112

equ	trap_EA_EndRegular                    -113
equ	trap_EA_GetInput                      -114
equ	trap_EA_ResetInput                    -115



equ	trap_BotLoadCharacter                 -116
equ	trap_BotFreeCharacter                 -117
equ	trap_Characteristic_Float             -118
equ	trap_Characteristic_BFloat            -119
equ	trap_Characteristic_Integer           -120
equ	trap_Characteristic_BInteger          -121
equ	trap_Characteristic_String            -122

equ	trap_BotAllocChatState                -123
equ	trap_BotFreeChatState                 -124
equ	trap_BotQueueConsoleMessage           -125
equ	trap_BotRemoveConsoleMessage          -126
equ	trap_BotNextConsoleMessage            -127
equ	trap_BotNumConsoleMessages            -128
equ	trap_BotInitialChat                   -129
equ	trap_BotReplyChat                     -130
equ	trap_BotChatLength                    -131
equ	trap_BotEnterChat                     -132
equ	trap_StringContains                   -133
equ	trap_BotFindMatch                     -134
equ	trap_BotMatchVariable                 -135
equ	trap_UnifyWhiteSpaces                 -136
equ	trap_BotReplaceSynonyms               -137
equ	trap_BotLoadChatFile                  -138
equ	trap_BotSetChatGender                 -139
equ	trap_BotSetChatName                   -140

equ	trap_BotResetGoalState                -141
equ	trap_BotResetAvoidGoals               -142
equ	trap_BotPushGoal                      -143
equ	trap_BotPopGoal                       -144
equ	trap_BotEmptyGoalStack                -145
equ	trap_BotDumpAvoidGoals                -146
equ	trap_BotDumpGoalStack                 -147
equ	trap_BotGoalName                      -148
equ	trap_BotGetTopGoal                    -149
equ	trap_BotGetSecondGoal                 -150
equ	trap_BotChooseLTGItem                 -151
equ	trap_BotChooseNBGItem                 -152
equ	trap_BotTouchingGoal                  -153
equ	trap_BotItemGoalInVisButNotVisible    -154
equ	trap_BotGetLevelItemGoal              -155
equ	trap_BotAvoidGoalTime                 -156
equ	trap_BotInitLevelItems                -157
equ	trap_BotUpdateEntityItems             -158
equ	trap_BotLoadItemWeights               -159
equ	trap_BotFreeItemWeights               -160
equ	trap_BotSaveGoalFuzzyLogic            -161
equ	trap_BotAllocGoalState                -162
equ	trap_BotFreeGoalState                 -163

equ	trap_BotResetMoveState                -164
equ	trap_BotMoveToGoal                    -165
equ	trap_BotMoveInDirection               -166
equ	trap_BotResetAvoidReach               -167
equ	trap_BotResetLastAvoidReach           -168
equ	trap_BotReachabilityArea              -169
equ	trap_BotMovementViewTarget            -170
equ	trap_BotAllocMoveState                -171
equ	trap_BotFreeMoveState                 -172
equ	trap_BotInitMoveState                 -173

equ	trap_BotChooseBestFightWeapon         -174
equ	trap_BotGetWeaponInfo                 -175
equ	trap_BotLoadWeaponWeights             -176
equ	trap_BotAllocWeaponState              -177
equ	trap_BotFreeWeaponState               -178
equ	trap_BotResetWeaponState              -179
equ	trap_GeneticParentsAndChildSelection  -180
equ	trap_BotInterbreedGoalFuzzyLogic      -181
equ	trap_BotMutateGoalFuzzyLogic          -182
equ	trap_BotGetNextCampSpotGoal           -183
equ	trap_BotGetMapLocationGoal            -184
equ	trap_BotNumInitialChats               -185
equ	trap_BotGetChatMessage                -186
equ	trap_BotRemoveFromAvoidGoals          -187
equ	trap_BotPredictVisiblePosition        -188
equ	trap_BotSetAvoidGoalTime              -189
equ	trap_BotAddAvoidSpot                  -190
equ	trap_AAS_AlternativeRouteGoals        -191
equ	trap_AAS_PredictRoute                 -192
equ	trap_AAS_PointReachabilityAreaIndex   -193

equ	trap_BotLibLoadSource                 -194
equ	trap_BotLibFreeSource                 -195
equ	trap_BotLibReadToken                  -196
equ	trap_BotLibSourceFileAndLine          -197
 
