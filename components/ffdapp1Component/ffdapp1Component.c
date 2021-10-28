
// ffdapp1Component.c
//
// Specifies the main source file of the component. Add initialization and event registrations to
// the files COMPONENT_INIT functions.


// Include the core framework C APIs.
#include "legato.h"

// Include your component's API interfaces.
#include "interfaces.h"


// This function is called only once on startup.  Place your initialization and event registration
// here.
COMPONENT_INIT
{
    // Write in the log that this component has started.
    LE_INFO("Component ffdapp1Component started.");
}
