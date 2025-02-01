# (C) 2018 - 2024 by Quirin Meyer
# quirin.meyer@hs-coburg.de
#
# Create assignment stubs to hand out to students.
#
# In your C++ files, mark solutions and stubs as follows
# #ifdef STUB
#   // Stub: Code for student to see
# #else
#   // Solution: Secret solution; not for students to see.
# #endif
# or
# #ifndef STUB
#   // Solution: Secret solution; not for students to see.
# #else
#   // Stub: Code for student to see
# #endif
#
# Requirements
# - to run the script, call Set-ExecutionPolicy RemoteSigned in a powershell as administrator.
# - You need to get the program unidef https://dotat.at/prog/unifdef/ and install it at location specified in the variable $unifdef

$unifdef = "C:\Program Files (x86)\unifdef\bin\win32\unifdef.exe"
$targetDirectory = 'stub'

$fileTypes="*.cpp","*.h", "*.hlsl", "*.hpp", "*.h'"

Remove-Item $targetDirectory -Recurse -ErrorAction Ignore

$files = Get-ChildItem -Recurse -Include $fileTypes | Resolve-Path -Relative 

for($i = 0; $i -lt $files.Length; $i++)
{
  $inFile=$files[$i]
  $outFile=$targetDirectory + "\" + $inFile
  New-Item $outFile -ItemType file -Force | Out-Null
  & $unifdef -DSTUB -o $outFile $inFile
  if("$LastExitCode" -eq 0)
  {
    Copy-Item $inFile $outFile
  }
}

Copy-Item CMakeLists.txt $targetDirectory/.
if (Test-Path doc)
{
	New-Item $targetDirectory/doc/ -ItemType Directory -Force  | Out-Null
	Copy-Item doc/*.pdf "$targetDirectory/doc/." -Force
}

if (Test-Path data)
{
	New-Item $targetDirectory/data/ -ItemType Directory -Force  | Out-Null
	Copy-Item data/* "$targetDirectory/data/." -Recurse
}


$archiveName = (Get-Item -Path ".\").BaseName + ".zip"

Compress-Archive -Force -Path "$targetDirectory/*"  -CompressionLevel optimal -DestinationPath $archiveName

Remove-Item $targetDirectory -Recurse -ErrorAction Ignore
