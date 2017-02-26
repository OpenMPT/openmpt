#!/usr/bin/env sh

# Usage:
# dailog.sh [tui|gui] [--infobox|--messagebox|--yesno|--gauge] title message
# WARNING: no error checking is done

fake_dialog () {
	case "$2" in
		--infobox)
			echo ""
			echo "$3"
			echo "$4"
			echo ""
			;;
		--msgbox)
			echo ""
			echo "$3"
			echo "$4"
			read -p "Press [Enter] key to continue ... \$ " DIALOG_RESULT
			echo ""
			;;
		--yesno)
			while true ; do
				echo ""
				echo "$3"
				echo "$4"
				read -p "([yes], no) \$ " DIALOG_RESULT
				echo ""
				case "x$DIALOG_RESULT" in
					xno)
						DIALOG_RESULT="n"
						break
						;;
					xNO)
						DIALOG_RESULT="n"
						break
						;;
					xyes)
						DIALOG_RESULT="y"
						break
						;;
					xYES)
						DIALOG_RESULT="y"
						break
						;;
					x)
						DIALOG_RESULT="y"
						break
						;;
					*)
						;;
				esac
			done
			case "$DIALOG_RESULT" in
				n)
					(exit 1)
					;;
				y)
					(exit 0)
					;;
				*)
					(exit 0)
					;;
			esac
			;;
		--textbox)
			echo ""
			echo "$3"
			if command -v "less" 2>/dev/null 1>/dev/null ; then
				less "$4"
			else
				if command -v "more" 2>/dev/null 1>/dev/null ; then
					more "$4"
				else
					cat "$4"
					read -p "Press [Enter] key to continue ... \$ " DIALOG_RESULT
				fi
			fi
			echo ""
			;;
		*)
			echo "$4"
			;;
	esac
}

fake_progress () {
	echo ""
	echo "$3"
	echo "$4"
	echo -n "0%..."
	while IFS='' read -r line ; do
		if [ '(' "$line" -gt 1 ')' -a '(' "$line" -lt 100 ')' ]; then
	    echo -n "$line%..."
		fi
	done
	echo -n "100%"
	echo ""
	echo ""
}

if [ "$2" = "--gauge" ]; then
  if [ "$1" = "tui" ]; then
		DIALOG_LIST="dialog whiptail gdialog xdialog fake_progress"
	else
		DIALOG_LIST="zenity gdialog xdialog dialog whiptail fake_progress"
	fi
	for d in $DIALOG_LIST ; do
		if [ "$d" = "fake_progress" ]; then
			fake_progress "tui" "$2" "$3" "$4"
			exit $?
		else
			if command -v "$d" 2>/dev/null 1>/dev/null ; then
				if [ "$d" = "zenity" ]; then
					exec $d --title "$3" --auto-close --progress "--text=$4" 0 0 0
				else
					exec $d --title "$3" "$2" "$4" 0 0 0
				fi
			fi
		fi
	done
else
  if [ "$1" = "tui" ]; then
		DIALOG_LIST="dialog whiptail gdialog xdialog fake_dialog"
	else
		DIALOG_LIST="gdialog xdialog dialog whiptail fake_dialog"
	fi
	for d in $DIALOG_LIST ; do
		if [ "$d" = "fake_dialog" ]; then
			fake_dialog "tui" "$2" "$3" "$4"
			exit $?
		else
			if command -v "$d" 2>/dev/null 1>/dev/null ; then
				exec $d --title "$3" "$2" "$4" 0 0
			fi
		fi
	done
fi
