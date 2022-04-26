#ifndef LOLLIPOP_UTILITY_H
#define LOLLIPOP_UTILITY_H

extern int exec_command(char * in, char * out, int outLen);
extern int wait_for_property(const char *name, const char *desired_value);

#endif //LOLLIPOP_UTILITY_H
