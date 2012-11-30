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
equ	trap_Cvar_InfoStringBuffer            -10
equ	trap_Argc                             -11
equ	trap_Argv                             -12
equ	trap_Cmd_ExecuteText                  -13
equ	trap_FS_FOpenFile                     -14
equ	trap_FS_Read                          -15
equ	trap_FS_Write                         -16
equ	trap_FS_FCloseFile                    -17
equ	trap_FS_GetFileList                   -18
equ	trap_R_RegisterModel                  -19
equ	trap_R_RegisterSkin                   -20
equ	trap_R_RegisterShaderNoMip            -21
equ	trap_R_ClearScene                     -22
equ	trap_R_AddRefEntityToScene            -23
equ	trap_R_AddPolyToScene                 -24
equ	trap_R_AddLightToScene                -25
equ	trap_R_RenderScene                    -26
equ	trap_R_SetColor                       -27
equ	trap_R_DrawStretchPic                 -28
equ	trap_UpdateScreen                     -29
equ	trap_CM_LerpTag                       -30
equ	trap_CM_LoadModel                     -31
equ	trap_S_RegisterSound                  -32
equ	trap_S_StartLocalSound                -33
equ	trap_S_StopBackgroundTrack            -34
equ	trap_S_StartBackgroundTrack           -35
equ	trap_Key_KeynumToStringBuf            -36
equ	trap_Key_GetBindingBuf                -37
equ	trap_Key_SetBinding                   -38
equ	trap_Key_IsDown                       -39
equ	trap_Key_GetOverstrikeMode            -40
equ	trap_Key_SetOverstrikeMode            -41
equ	trap_Key_ClearStates                  -42
equ	trap_Key_GetCatcher                   -43
equ	trap_Key_SetCatcher                   -44
equ	trap_GetClipboardData                 -45
equ	trap_GetGlconfig                      -46
equ	trap_GetClientState                   -47
equ	trap_GetConfigString                  -48
equ	trap_LAN_GetPingQueueCount            -49
equ	trap_LAN_ClearPing                    -50
equ	trap_LAN_GetPing                      -51
equ	trap_LAN_GetPingInfo                  -52
equ	trap_Cvar_Register                    -53
equ	trap_Cvar_Update                      -54
equ	trap_MemoryRemaining                  -55
equ	trap_R_RegisterFont                   -56
equ	trap_R_ModelBounds                    -57
equ	trap_RealTime                         -58
equ	trap_LAN_GetServerCount               -59
equ	trap_LAN_GetServerAddressString       -60
equ	trap_LAN_GetServerInfo                -61
equ	trap_LAN_MarkServerVisible            -62
equ	trap_LAN_UpdateVisiblePings           -63
equ	trap_LAN_ResetPings                   -64
equ	trap_LAN_LoadCachedServers            -65
equ	trap_LAN_SaveCachedServers            -66
equ	trap_LAN_AddServer                    -67
equ	trap_LAN_RemoveServer                 -68
equ	trap_CIN_PlayCinematic                -69
equ	trap_CIN_StopCinematic                -70
equ	trap_CIN_RunCinematic                 -71
equ	trap_CIN_DrawCinematic                -72
equ	trap_CIN_SetExtents                   -73
equ	trap_R_RemapShader                    -74
equ	trap_LAN_ServerStatus                 -75
equ	trap_LAN_GetServerPing                -76
equ	trap_LAN_ServerIsVisible              -77
equ	trap_LAN_CompareServers               -78
equ	trap_FS_Seek                          -79

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

