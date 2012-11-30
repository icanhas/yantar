code

equ	trap_Error                            -1
equ	trap_Print                            -2
equ	trap_Milliseconds                     -3
equ	trap_Cvar_Set                         -4
equ	trap_Cvar_VariableValue               -5
equ	trap_Cvar_VariableStringBuffer        -6
equ	trap_Cvar_SetValue                    -7
equ	trap_Cvar_Reset                       -8
equ	trap_Cvar_Create                      -9
equ	trap_Cvar_Register                    -10
equ	trap_Cvar_Update                      -11
equ	trap_Cvar_InfoStringBuffer            -12
equ	trap_Argc                             -13
equ	trap_Argv                             -14
equ	trap_Cmd_ExecuteText                  -15
equ	trap_FS_FOpenFile                     -16
equ	trap_FS_Seek                          -17
equ	trap_FS_Read                          -18
equ	trap_FS_Write                         -19
equ	trap_FS_FCloseFile                    -20
equ	trap_FS_GetFileList                   -21
equ	trap_R_RegisterModel                  -22
equ	trap_R_RegisterSkin                   -23
equ	trap_R_RegisterShaderNoMip            -24
equ	trap_R_RegisterFont                   -25
equ	trap_R_RemapShader                    -26
equ	trap_R_ModelBounds                    -27
equ	trap_R_ClearScene                     -28
equ	trap_R_AddRefEntityToScene            -29
equ	trap_R_AddPolyToScene                 -30
equ	trap_R_AddLightToScene                -31
equ	trap_R_RenderScene                    -32
equ	trap_R_SetColor                       -33
equ	trap_R_DrawStretchPic                 -34
equ	trap_UpdateScreen                     -35
equ	trap_CM_LerpTag                       -36
equ	trap_CM_LoadModel                     -37
equ	trap_S_RegisterSound                  -38
equ	trap_S_StartLocalSound                -39
equ	trap_S_StopBackgroundTrack            -40
equ	trap_S_StartBackgroundTrack           -41
equ	trap_Key_KeynumToStringBuf            -42
equ	trap_Key_GetBindingBuf                -43
equ	trap_Key_SetBinding                   -44
equ	trap_Key_IsDown                       -45
equ	trap_Key_GetOverstrikeMode            -46
equ	trap_Key_SetOverstrikeMode            -47
equ	trap_Key_ClearStates                  -48
equ	trap_Key_GetCatcher                   -49
equ	trap_Key_SetCatcher                   -50
equ	trap_GetClipboardData                 -51
equ	trap_GetGlconfig                      -52
equ	trap_GetClientState                   -53
equ	trap_GetConfigString                  -54
equ	trap_MemoryRemaining                  -55
equ	trap_RealTime                         -56
equ	trap_LAN_GetPingQueueCount            -57
equ	trap_LAN_ClearPing                    -58
equ	trap_LAN_GetPing                      -59
equ	trap_LAN_GetPingInfo                  -60
equ	trap_LAN_GetServerCount               -61
equ	trap_LAN_GetServerAddressString       -62
equ	trap_LAN_GetServerInfo                -63
equ	trap_LAN_MarkServerVisible            -64
equ	trap_LAN_UpdateVisiblePings           -65
equ	trap_LAN_ResetPings                   -66
equ	trap_LAN_LoadCachedServers            -67
equ	trap_LAN_SaveCachedServers            -68
equ	trap_LAN_AddServer                    -69
equ	trap_LAN_RemoveServer                 -70
equ	trap_LAN_ServerStatus                 -71
equ	trap_LAN_GetServerPing                -72
equ	trap_LAN_ServerIsVisible              -73
equ	trap_LAN_CompareServers               -74
equ	trap_CIN_PlayCinematic                -75
equ	trap_CIN_StopCinematic                -76
equ	trap_CIN_RunCinematic                 -77
equ	trap_CIN_DrawCinematic                -78
equ	trap_CIN_SetExtents                   -79

equ	memset                                -80
equ	memcpy                                -81
equ	strncpy                               -82
equ	sin                                   -83
equ	cos                                   -84
equ	atan2                                 -85
equ	sqrt                                  -86
equ	floor                                 -87
equ	ceil                                  -88
equ	acos                                  -89
equ	asin                                  -90
equ	atan                                  -91

