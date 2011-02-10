'
' Tests for the BuildNumber utility
'

all_was_ok = true

' Simple test - display full version number
options = "--display f"
expected_output = "2.9.9.1" 
expected_contents = ""
filename = ""
all_was_ok = all_was_ok And DoTest(options, expected_output, expected_contents, filename)

' Simple test - display short version number
options = "--display s"
expected_output = "2.9.9" 
expected_contents = ""
filename = ""
all_was_ok = all_was_ok And DoTest(options, expected_output, expected_contents, filename)

' Simple test - read from another file
options = "--display s --file test1.xml"
expected_output = "1.0.0" 
expected_contents = ""
filename = ""
all_was_ok = all_was_ok And DoTest(options, expected_output, expected_contents, filename)

' Extensive, real-life test - from MCK.
options = "--display f --output version.h --programming-language c --variable-name g_MCKVersion"
expected_output = ""
expected_contents = "const char g_MCKVersion[] = ""2.9.9.1"";"
filename = "version.h"
all_was_ok = all_was_ok And DoTest(options, expected_output, expected_contents, filename)

' Run this one twice - it check if the not-overwriting-of-old-file-with-same-content works 
all_was_ok = all_was_ok And DoTest(options, expected_output, expected_contents, filename)



If all_was_ok Then
	WScript.Echo "All tests passed."
Else
	WScript.Echo "There were errors."
End If

Function DoTest(options, expected_output, expected_contents, filename)
	errors = false
	command = "BuildNumber "
	command = command + options
	WScript.Echo "Running command " + command
	output = RunSilent(command)
	If filename <> "" Then 
		filecontent = GetContent(filename)
	Else
		filecontent = ""
	End If
	
	If output <> expected_output Then
		WScript.Echo "Failed: output != expected output"	
		WScript.Echo "Output:"
		WScript.Echo output
		WScript.Echo "Expected output:"
		WScript.Echo expected_output
		errors = true
	End If
	
	If filecontent <> expected_contents Then
		WScript.Echo "Failed: filecontent != expected_contents"	
		WScript.Echo "Filecontent:"
		WScript.Echo filecontent
		WScript.Echo "Expected Contents:"
		WScript.Echo expected_contents
		errors = true
	End If

	If errors Then
		DoTest = false
	Else
		DoTest = true
	End If

	WScript.Echo "====================================================="
End Function

Function GetContent(filename)
	Const ForReading = 1
	dim fso, f
	set fso = WScript.CreateObject("Scripting.FileSystemObject")
	set f = fso.OpenTextFile(filename, ForReading)
	
	If f.AtEndOfStream Then
		WScript.Echo "ERROR: could not read from " + filename + " or file empty."
	Else
		GetContent = f.ReadAll
	End If
	
	f.Close
End Function

Function Run(ByVal cmd)
    dim shell, wsh_script_exec
    set shell = WScript.CreateObject("Wscript.Shell")
    set wsh_script_exec = shell.Exec(cmd)
    total_stdout_result = ""
    Do
        Dim status
        status = wsh_script_exec.Status
        stdout_result = wsh_script_exec.StdOut.ReadAll()
        WScript.StdOut.Write stdout_result
        WScript.StdErr.Write wsh_script_exec.StdErr.ReadAll()
        If Status <> 0 Then Exit Do
        WScript.Sleep 10
        total_stdout_result = total_stdout_result + stdout_result
    Loop
   Run = total_stdout_result
End Function

Function RunSilent(ByVal cmd)
    dim shell, wsh_script_exec
    set shell = WScript.CreateObject("Wscript.Shell")
    set wsh_script_exec = shell.Exec(cmd)
    total_stdout_result = ""
    Do
        Dim status
        status = wsh_script_exec.Status
        stdout_result = wsh_script_exec.StdOut.ReadAll()
        If Status <> 0 Then Exit Do
        WScript.Sleep 10
        total_stdout_result = total_stdout_result + stdout_result
    Loop
   RunSilent = total_stdout_result
End Function