#include "applicationClass.h"

#include "main.h"


int __cdecl main(void)
{
	applicationClass* App = new applicationClass();
	if (App)
	{
		App->Initialize();
		App->Run();
	}
	return 0;
}

