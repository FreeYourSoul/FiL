{rev ? "c178d7e0625c87188ebd2b1bcd8cfc482376f10b", stdenv, cmake, rocksdb, catch2, fmt}:

stdenv.mkDerivation rec {

    version = "1.0.0";
    name = "fil-${version}";

    src = builtins.fetchGit{
        url = "https://github.com/FreeYourSoul/Fil.git";
	rev = "${rev}";
    };

    cmakeFlags = "
    -DBUILD_SERVICE_WORLD=OFF
    -DBUILD_SERVICE_QUEST=OFF
    -DBUILD_SERVICE_ARENA=OFF
    -DBUILD_SERVICE_INVENTORY=OFF
    -DBUILD_DISP_CHAT=OFF
    -DBUILD_DISP_GATEWAY=OFF
    -DBUILD_DISPATCHER=ON
    ";

    buildInputs = [ cmake rocksdb catch2 fmt ];

}