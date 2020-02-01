@echo off
rem Generates C# protocol bindings for Metro UI to Service protocol

set _=%CD%

set OUT="..\Protocol\Auto.cs"

cd %~dp0

del /F %OUT%

ProtoGen csharp -f "%OUT%"^
 --id-scope=branch^
 --namespace-prefix=Pravala.Protocol.Auto^
 ../../proto/error.proto^
 ../../proto/ctrl.proto^
 ../../proto/log.proto

cd %_%
