#ifndef POWER_H
#define POWER_H

/*now work with power supply through ports, then it will be through acpi*/
/*This implementation is not safe at all, do not use!*/
/*was quickly made for shell*/

void shutdown(void);
void reboot(void);

#endif // POWER_H