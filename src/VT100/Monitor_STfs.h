#ifndef MONITOR_STfs_H
#define MONITOR_STfs_H

extern const T_VT100_Menu MENU_STfs;

void Do_STfs_init(uint8_t keycode);
void Do_STfs_list_files(uint8_t keycode);
void Do_STfs_performance_test(uint8_t keycode);

#endif  // MONITOR_STfs_H
