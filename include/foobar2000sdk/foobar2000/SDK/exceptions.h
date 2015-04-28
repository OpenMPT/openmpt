
//! Base class for exceptions that should show a human readable message when caught in app entrypoint function rather than crash and send a crash report.
PFC_DECLARE_EXCEPTION(exception_messagebox,pfc::exception,"Internal Error");

//! Base class for exceptions that should result in a quiet app shutdown.
PFC_DECLARE_EXCEPTION(exception_shutdownrequest,pfc::exception,"Shutdown Request");


PFC_DECLARE_EXCEPTION(exception_installdamaged, exception_messagebox, "Internal error - one or more of the installed components have been damaged.");
PFC_DECLARE_EXCEPTION(exception_osfailure, exception_messagebox, "Internal error - broken Windows installation?");
PFC_DECLARE_EXCEPTION(exception_out_of_resources, exception_messagebox, "Not enough system resources available.");

PFC_DECLARE_EXCEPTION(exception_configdamaged, exception_messagebox, "Internal error - configuration files are unreadable.");

PFC_DECLARE_EXCEPTION(exception_profileaccess, exception_messagebox, "Internal error - cannot access configuration folder.");
