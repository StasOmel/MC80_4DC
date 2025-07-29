#ifndef MONITOR_FILEX_H
#define MONITOR_FILEX_H

extern const T_VT100_Menu MENU_FileX;

void Do_FileX_init(uint8_t keycode);
void Do_FileX_list_files(uint8_t keycode);
void Do_FileX_performance_test(uint8_t keycode);

#endif  // MONITOR_FILEX_H
