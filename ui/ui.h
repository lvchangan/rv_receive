#ifndef LOLLIPOP_UI_H
#define LOLLIPOP_UI_H

extern void show_text(char* name, char* subname, char* data);
extern void show_image(const char* filePath,int x,int y);
extern void show_image_fullscreen(const char* filePath);
extern void show_image_fitscreen(const char* filePath);
extern void lollipop_ui_update();
extern void lollipop_ui_hide();
extern void lollipop_ui_show();

#endif //LOLLIPOP_UI_H

