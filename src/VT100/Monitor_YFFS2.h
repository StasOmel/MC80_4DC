#ifndef MONITOR_YFSS2_H
#define MONITOR_YFSS2_H

extern const T_VT100_Menu MENU_FileX;

void Do_YFFS2_init(uint8_t keycode);
void Do_YFFS2_list_files(uint8_t keycode);
void Do_YFFS2_performance_test(uint8_t keycode);

#endif  // MONITOR_YFSS2_H
