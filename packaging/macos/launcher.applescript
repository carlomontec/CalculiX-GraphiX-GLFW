on open theFiles
	set appPath to POSIX path of (path to me)
	set binPath to appPath & "Contents/MacOS/cgx_glfw_bin"
	repeat with aFile in theFiles
		set filePath to POSIX path of aFile
		set randTag to (do shell script "date +%s%N 2>/dev/null || date +%s")
		set cmdFile to "/tmp/cgx_" & randTag & ".command"
		set shScript to "#!/bin/bash" & linefeed & quoted form of binPath & " " & quoted form of filePath & linefeed & "rm -f " & quoted form of cmdFile & linefeed
		do shell script "printf '%s' " & quoted form of shScript & " > " & quoted form of cmdFile & " && chmod +x " & quoted form of cmdFile & " && open " & quoted form of cmdFile
	end repeat
end open

on run
	set appPath to POSIX path of (path to me)
	set binPath to appPath & "Contents/MacOS/cgx_glfw_bin"
	set randTag to (do shell script "date +%s%N 2>/dev/null || date +%s")
	set cmdFile to "/tmp/cgx_" & randTag & ".command"
	set shScript to "#!/bin/bash" & linefeed & quoted form of binPath & linefeed & "rm -f " & quoted form of cmdFile & linefeed
	do shell script "printf '%s' " & quoted form of shScript & " > " & quoted form of cmdFile & " && chmod +x " & quoted form of cmdFile & " && open " & quoted form of cmdFile
end run
