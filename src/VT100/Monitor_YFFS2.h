#ifndef MONITOR_YAFFS2_H
#define MONITOR_YAFFS2_H

extern const T_VT100_Menu MENU_YAFFS2;

void Do_YAFFS2_init(uint8_t keycode);
void Do_YAFFS2_list_files(uint8_t keycode);
void Do_YAFFS2_performance_test(uint8_t keycode);
void Do_YAFFS2_list_lost_found(uint8_t keycode);

#endif  // MONITOR_YAFFS2_H
