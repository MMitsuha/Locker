add_rules("mode.debug", "mode.release")

set_project("Locker")

add_requires("spdlog","fmt","vcpkg::lazy-importer")

if is_mode("release") then
    set_runtimes("MT")
    add_requireconfs("vcpkg::*",{vs_runtime = "MT",debug = false})
else
    set_runtimes("MTd")
    add_requireconfs("vcpkg::*",{vs_runtime = "MTd",debug = true})
end 

target("Locker")
    set_kind("binary")
    set_pcxxheader("src/AApch.h")
    add_files("src/*.cpp")
    add_packages("spdlog","fmt","vcpkg::lazy-importer")

--  add_syslinks("gdi32.lib","Secur32.lib","wbemuuid.lib","OleAut32.lib","Shell32.lib","ntdll.lib","user32.lib","Advapi32.lib")