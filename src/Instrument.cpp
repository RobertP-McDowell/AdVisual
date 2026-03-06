#include <Instrument.h>


Instrument* Bank::GetInstrumentByName(char name[9]) {
	for (Instrument& ins : instruments) {
		if (strcmp(name, ins.name) == 0) {
			return &ins;
		}
	}
	return nullptr;
}