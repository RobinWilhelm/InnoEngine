@echo off
title vcpkg Setup

echo Getting vcpkg files...
git clone https://github.com/microsoft/vcpkg.git

echo Installing vcpkg...
cd vcpkg
bootstrap-vcpkg.bat -disableMetrics