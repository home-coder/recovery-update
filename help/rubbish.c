// command line args come from, in decreasing precedence:
//   - the actual command line
//   - the bootloader control block (one per line, after "recovery")
//   - the contents of COMMAND_FILE (one per line)
static void
get_args(int *argc, char ***argv) {
	struct bootloader_message boot;
	memset(&boot, 0, sizeof(boot));
	get_bootloader_message(&boot);  // this may fail, leaving a zeroed structure
	stage = strndup(boot.stage, sizeof(boot.stage));

	if (boot.command[0] != 0 && boot.command[0] != 255) {
		LOGI("Boot command: %.*s\n", (int)sizeof(boot.command), boot.command);
	}

	if (boot.status[0] != 0 && boot.status[0] != 255) {
		LOGI("Boot status: %.*s\n", (int)sizeof(boot.status), boot.status);
	}

	// --- if arguments weren't supplied, look in the bootloader control block
	if (*argc <= 1) {
		boot.recovery[sizeof(boot.recovery) - 1] = '\0';  // Ensure termination
		const char *arg = strtok(boot.recovery, "\n");
		if (arg != NULL && !strcmp(arg, "recovery")) {
			*argv = (char **) malloc(sizeof(char *) * MAX_ARGS);
			(*argv)[0] = strdup(arg);
			for (*argc = 1; *argc < MAX_ARGS; ++*argc) {
				if ((arg = strtok(NULL, "\n")) == NULL) break;
				(*argv)[*argc] = strdup(arg);
			}
			LOGI("Got arguments from boot message\n");
		} else if (boot.recovery[0] != 0 && boot.recovery[0] != 255) {
			LOGE("Bad boot message\n\"%.20s\"\n", boot.recovery);
		}
	}

	// --- if that doesn't work, try the command file
	if (*argc <= 1) {
		FILE *fp = fopen_path(COMMAND_FILE, "r");
		if (fp != NULL) {
			char *token;
			char *argv0 = (*argv)[0];
			*argv = (char **) malloc(sizeof(char *) * MAX_ARGS);
			(*argv)[0] = argv0;  // use the same program name

			char buf[MAX_ARG_LENGTH];
			for (*argc = 1; *argc < MAX_ARGS; ++*argc) {
				if (!fgets(buf, sizeof(buf), fp)) break;
				token = strtok(buf, "\r\n");
				if (token != NULL) {
					(*argv)[*argc] = strdup(token);  // Strip newline.
				} else {
					--*argc;
				}
			}

			check_and_fclose(fp, COMMAND_FILE);
			LOGI("Got arguments from %s\n", COMMAND_FILE);
		}
	}

	// --> write the arguments we have back into the bootloader control block
	// always boot into recovery after this (until finish_recovery() is called)
	strlcpy(boot.command, "boot-recovery", sizeof(boot.command));
	strlcpy(boot.recovery, "recovery\n", sizeof(boot.recovery));
	int i;
	for (i = 1; i < *argc; ++i) {
		strlcat(boot.recovery, (*argv)[i], sizeof(boot.recovery));
		strlcat(boot.recovery, "\n", sizeof(boot.recovery));
	}
	load_keeping_bootloader_message(&boot);
	set_bootloader_message(&boot);
}
