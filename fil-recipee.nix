{branch ? "master",stdenv, cmake, rocksdb, catch2, fmt}:

stdenv.mkDerivation rec {

    version = "1.0.0";
    name = "fil-${version}";

    src = builtins.fetchGit{
        url = "https://github.com/FreeYourSoul/Fil.git";
        ref = "${branch}";
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