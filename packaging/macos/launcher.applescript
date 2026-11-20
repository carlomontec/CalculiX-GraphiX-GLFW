on open theFiles
	set appPath to POSIX path of (path to me)
	set binPath to appPath & "Contents/MacOS/cgx_glfw_bin"
	repeat with aFile in theFiles
		set filePath to POSIX path of aFile
		set randTag to (do shell script "uuidgen")
		set cmdFile to "/tmp/cgx_" & randTag & ".command"
		set fileDir to (do shell script "dirname " & quoted form of filePath)
		set fileExt to (do shell script "f=" & quoted form of filePath & "; b=\"${f##*/}\"; case \"$b\" in *.*) printf '%s' \"${b##*.}\" | tr '[:upper:]' '[:lower:]' ;; esac")
		set launchFlag to ""
		if fileExt is "fbl" or fileExt is "fbd" then
			set launchFlag to "-b "
		else if fileExt is "inp" then
			set launchFlag to "-c "
		else if fileExt is "stl" then
			set launchFlag to "-stl "
		else if fileExt is "vtk" then
			set launchFlag to "-vtk "
		end if
		set shScript to "#!/bin/bash" & linefeed & "trap 'rm -f \"$0\"' EXIT" & linefeed & "cd " & quoted form of fileDir & " || exit 1" & linefeed & quoted form of binPath & " " & launchFlag & quoted form of filePath & linefeed
		do shell script "printf '%s' " & quoted form of shScript & " > " & quoted form of cmdFile & " && chmod +x " & quoted form of cmdFile & " && open " & quoted form of cmdFile
	end repeat
end open

on run
	set appPath to POSIX path of (path to me)
	set binPath to appPath & "Contents/MacOS/cgx_glfw_bin"
	set randTag to (do shell script "uuidgen")
	set cmdFile to "/tmp/cgx_" & randTag & ".command"
	set shScript to "#!/bin/bash" & linefeed & "trap 'rm -f \"$0\"' EXIT" & linefeed & quoted form of binPath & linefeed
	do shell script "printf '%s' " & quoted form of shScript & " > " & quoted form of cmdFile & " && chmod +x " & quoted form of cmdFile & " && open " & quoted form of cmdFile
end run
