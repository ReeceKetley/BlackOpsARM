# SDK470 registered menu versus UI context diagnostic

Read-only source audit, no production/build/device/Ledger changes. Root actual SDK470/stage42 PID28098 BuildId b1a729750397ae1b463e0dfab606091d97232638 now proves genuine ui/menus.txt, ui/patch_menus.txt and main/main_text/main_setup/main_sp/main_lockout/levels_zombie asset registrations. ui_readingSaveDevice is0. Earlier asset-request warnings alone no longer support absence of genuine menus.

One diagnostic batch through the working slash-prefixed console, after checking ui_showMenuOnly:

`/uiscript_debug 1; ui_showList 1; openmenuimmediate main_text`

The debug dvars are existing unrestricted registrations (ui_main.cpp:3348 and3437); they expose canonical visibility decisions and painted-menu names. The requested command is registered in UI_RegisterCmds, ui_shared.cpp:3127. UI_OpenMenuImmediate_f line3387 uses client0's actual UI_GetInfo(0)->uiDC. Menus_FindByName line1050 scans that context's menuCount/Menus array only, without DB lookup. Menus_OpenImmediateByName line4493 emits Could not find menu 'main_text' when that context lookup fails. With the proven registry entry, a fresh such warning distinguishes context admission from asset registration.

If found, Menus_OpenImmediateByName disables slide/fade admission and calls canonical Menus_Open. It adds the real menu to the stack, sets visible/focus flags6 in Menus_SetupOpenMenu (line4511), and runs actual focus/onOpen events. It does not override owner-draw/show-hide/visible expressions or individual item gates. If Menu_IsVisible line7910 fails its visible expression, uiscript_debug logs the existing hiding-the-main_text/visible-when-false message at line7954. ui_showList appends main_text only when Menu_Paint returns1 (line10862 or10942), then paints the list through Menu_PaintAll_DrawVisibleList line10749.

Interpret only observed outcomes: fresh missing-menu warning means context absence; the explicit hiding diagnostic means the menu visible expression is false; main_text in the onscreen visible list proves menu-level paint admission, not individual text drawing. If ordinary labels appear only after the explicit open, inspect the original title opening/event sequence. If the list includes main_text but its text is absent, inspect that menu's individual item visibility/foreColor/text/display state. No missing warning and no list entry leaves the earlier menu visibility/onOpen/item-admission gates to inspect; it does not establish a missing asset or exact cause. Restore debug dvars to their prior values after the bounded capture.

Six relevant command/lookup/setup/visibility/list-drawing bodies match BlackOps64 ignoring whitespace; source-proof.json and extracted bodies are retained in native_frontend_title_menu_context1. This agent has not executed the diagnostic or observed its live result.
