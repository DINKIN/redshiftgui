/**\file		redshiftgui.c
 * \author		Mao Yu
 * \date		Friday, June 11, 2010
 * \brief		Main code
 * \details
 * This code is forked from the redshift project
 * (https://bugs.launchpad.net/redshift) by:
 * Jon Lund Steffensen.
 *
 * The license for this project as a whole is same (GPL v3),
 * although some components of this code (such as argument parsing)
 * were originally under different license.
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "common.h"
#include "argparser.h"
#include "options.h"
#include "gamma.h"
#include "solar.h"
#include "systemtime.h"
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef ENABLE_RANDR
# include "backends/randr.h"
#endif
#ifdef ENABLE_VIDMODE
# include "backends/vidmode.h"
#endif
#ifdef ENABLE_WINGDI
# include "backends/w32gdi.h"
#endif

// Main program return codes
#define RET_MAIN_OK 0
#define RET_MAIN_ERR -1

// Internal function to parse arguments
static int _parse_options(int argc, char *argv[]){
	args_addarg("c","crt",
		_("<CRTC> CRTC to apply adjustment to (RANDR only)"),ARGVAL_STRING);
	args_addarg("g","gamma",
		_("<R:G:B> Additional gamma correction to apply"),ARGVAL_STRING);
	args_addarg("l","latlon",
		_("<LAT:LON> Latitude and longitude"),ARGVAL_STRING);
	args_addarg("m","method",
		_("<METHOD> Method to use (RANDR, VidMode, or WinGDI"),ARGVAL_STRING);
	args_addarg("n","no-gui",
		_("Run in console mode (no GUI)."),ARGVAL_NONE);
	args_addarg("o","oneshot",
		_("Adjust color and then exit (no GUI)"),ARGVAL_NONE);
	args_addarg("r","speed",
		_("<SPEED> Transition speed (default 100 K/s)"),ARGVAL_STRING);
	args_addarg("s","screen",
		_("<SCREEN> Screen to apply to"),ARGVAL_STRING);
	args_addarg("t","temps",
		_("<DAY:NIGHT> Color temperature to set at daytime/night"),ARGVAL_STRING);
	args_addarg("v","verbose",
		_("<LEVEL> Verbosity of output (0 for regular, 1 for more)"),ARGVAL_STRING);
	args_addarg("h","help",
		_("Display this help message"),ARGVAL_NONE);
	if( (args_parse(argc,argv) != ARGRET_OK) ){
		LOG(LOGERR,_("Error occurred parsing options,"
					"Check your config file or command line."));
		return RET_FUN_FAILED;
	}
	else{
		char *val;
		int err=0;
		char Config_file[LONGEST_PATH];
		
		if( opt_get_config_file(Config_file,LONGEST_PATH)
				&& ((args_parsefile(Config_file)) != ARGRET_OK) )
			LOG(LOGWARN,_("Invalid/empty config: %s"),Config_file);

		if( args_check("h") ){
			printf(_("Redshift GUI help:\n"));
			args_print();
			return RET_FUN_FAILED;
		}

		opt_set_defaults();
		if( (val=args_getnamed("c")) )
			err = (!opt_set_crtc(atoi(val))) || err;
		if( (val=args_getnamed("g")) )
			err = (!opt_parse_gamma(val)) || err;
		if( (val=args_getnamed("l")) )
			err = (!opt_parse_location(val)) || err;
		if( (val=args_getnamed("n")) )
			err = (!opt_set_nogui(1)) || err;
		if( (val=args_getnamed("m")) )
			err = (!opt_parse_method(val)) || err;
		if( (val=args_getnamed("o")) )
			err = (!opt_set_oneshot(1) ) || err;
		if( (val=args_getnamed("r")) )
			err = (!opt_set_transpeed(atoi(val))) || err;
		if( (val=args_getnamed("s")) )
			err = (!opt_set_screen(atoi(val))) || err;
		if( (val=args_getnamed("t")) )
			err = (!opt_parse_temperatures(val)) || err;
		if( (val=args_getnamed("v")) )
			err = (!opt_set_verbose(atoi(val))) || err;
		if( err ){
			return RET_FUN_FAILED;
		}
		if( args_unknown() ){
			printf(_("Unknown arguments encountered.\n"));
			return RET_FUN_FAILED;
		}
	}
	return RET_FUN_SUCCESS;
}

/* Change gamma and exit. */
static int _do_oneshot(void){
	double now, elevation;
	int temp;
	gamma_method_t method = opt_get_method();

	if ( systemtime_get_time(&now) ){
		LOG(LOGERR,_("Unable to read system time."));
		return RET_FUN_FAILED;
	}

	/* Current angular elevation of the sun */
	elevation = solar_elevation(now, opt_get_lat(), opt_get_lon());

	/* TRANSLATORS: Append degree symbol if possible. */
	LOG(LOGINFO,_("Solar elevation: %f"),elevation);

	/* Use elevation of sun to set color temperature */
	temp = gamma_calc_temp(elevation, opt_get_temp_day(),
			opt_get_temp_night());

	LOG(LOGINFO,_("Color temperature: %uK"), temp);

	gamma_state_get_temperature(method);
	/* Adjust temperature */
	if ( !gamma_state_set_temperature(method, temp, opt_get_gamma()) ){
		LOG(LOGERR,_("Temperature adjustment failed."));
		return RET_FUN_FAILED;
	}
	return RET_FUN_SUCCESS;
}

#ifdef _WIN32
	static int exiting=0;
	/* Signal handler for exit signals */
	BOOL CtrlHandler( DWORD fdwCtrlType ){
		switch( fdwCtrlType ){
		case CTRL_C_EVENT:
			LOG(LOGINFO,_("Ctrl-C event."));
			exiting=1;
			return( TRUE );
		// CTRL-CLOSE: confirm that the user wants to exit.
		case CTRL_CLOSE_EVENT:
			LOG(LOGINFO,_("Ctrl-Close event."));
			exiting=1;
			return( TRUE );
		// Pass other signals to the next handler.
		case CTRL_BREAK_EVENT:
			LOG(LOGINFO,_("Ctrl-Break event."));
			return FALSE;
		case CTRL_LOGOFF_EVENT:
			LOG(LOGINFO,_("Ctrl-Logoff event."));
			return FALSE;
		case CTRL_SHUTDOWN_EVENT:
			LOG(LOGINFO,_("Ctrl-Shutdown event."));
			return FALSE;
		default:
			return FALSE;
		}
	}
	/* Register signal handler */
	static void sig_register(void){
		if( !SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) )
			LOG(LOGERR,_("Unable to register Control Handler."));
	}
#elif defined(HAVE_SYS_SIGNAL_H)
	static volatile sig_atomic_t exiting = 0;
	/* Signal handler for exit signals */
	static void
	sigexit(int signo)
	{exiting = 1;}
	/* Register signal handler */
	static void sig_register(void){
		struct sigaction sigact;
		sigact.sa_handler = sigexit;
		sigemptyset(&sigact.sa_mask);
		sigact.sa_flags = 0;
		sigaction(SIGINT, &sigact, NULL);
		sigaction(SIGTERM, &sigact, NULL);
	}
#else /* ! HAVE_SYS_SIGNAL_H */
	static int exiting = 0;
#	define sig_register()
#endif /* ! HAVE_SYS_SIGNAL_H */
/* Change gamma continuously until break signal. */
static int _do_console(void)
{
	sig_register();
	while(!exiting){
#ifndef _WIN32
		usleep(5000000);
#else /* ! _WIN32 */
		Sleep(5000);
#endif
	}
	return RET_FUN_SUCCESS;
}

int main(int argc, char *argv[]){
	gamma_method_t method;
	int ret;

	if( log_init(NULL,LOGBOOL_FALSE,NULL) != LOGRET_OK ){
		printf(_("Could not initialize logger.\n"));
		return RET_MAIN_ERR;
	}

	if( !(_parse_options(argc,argv)) ){
		args_free();
		log_end();
		return RET_MAIN_ERR;
	}
	method = gamma_init_method(opt_get_screen(),opt_get_crtc(),
			opt_get_method());
	if( !method ){
		args_free();
		log_end();
		return RET_MAIN_ERR;
	}
	opt_set_method(method);
	
	if(opt_get_oneshot()){
		// One shot mode
		LOG(LOGINFO,_("Doing one-shot adjustment."));
		ret = _do_oneshot();
	}else if(opt_get_nogui()){
		// Console mode
		LOG(LOGINFO,_("Starting in console mode."));
		ret = _do_console();
	}else{
		// GUI mode
		LOG(LOGINFO,_("Starting in GUI mode."));
#if defined(ENABLE_IUP)

#elif defined(ENABLE_GTK)
	
#else
		LOG(LOGERROR,_("No GUI toolkit compiled in."));
		ret = RET_FUN_FAILED;
#endif
	}
	gamma_state_free(opt_get_method());

	// Else we go to GUI mode
	args_free();
	log_end();
	if( ret )
		return RET_MAIN_OK;
	else
		return RET_MAIN_ERR;
}

#ifdef _WIN32
// Win32 wrapper function for GUI mode
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		LPSTR lpCmdLine, int nCmdShow){
	extern char ** __argv;
	extern int __argc;

	// This attaches a console to the parent process if it has a console
	if(AttachConsole(ATTACH_PARENT_PROCESS)){
		// reopen stout handle as console window output
		freopen("CONOUT$","wb",stdout);
		// reopen stderr handle as console window output
		freopen("CONOUT$","wb",stderr);
	}
	return main(__argc,__argv);
}
#endif //_WIN32
