{ lib
, stdenv
, cmake
, ninja
, gcc
, catch2_3
, fmt
, rocksdb
, lcov
, gcovr
, execute_test ? false
, with_coverage ? false
}:
let
    version = builtins.readFile ../VERSION; 
in
stdenv.mkDerivation rec {
  pname = "fil";
  inherit version;

  src = ../.;

  nativeBuildInputs = [
    cmake
    ninja
    gcc
  ] ++ lib.optionals execute_test [
    lcov
    gcovr
  ];

  buildInputs = [
    catch2_3
    fmt
    rocksdb
  ];

  cmakeFlags = [
    "-DBUILD_TESTING=${if execute_test || with_coverage then "ON" else "OFF"}"
    "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
    "-DCMAKE_INSTALL_INCLUDEDIR=${placeholder "out"}/include"
    "-DCMAKE_INSTALL_LIBDIR=${placeholder "out"}/lib"
  ];

  CXXFLAGS = lib.optionalString with_coverage "-coverage -fkeep-inline-functions -fno-inline -fno-inline-small-functions -fno-default-inline -O0 -g";

  doCheck = execute_test;

  postCheck = lib.optionalString with_coverage ''
    echo "Generating coverage report..."

    mkdir -p $out/coverage

    cp $sourceDir/

    # During a Nix sandbox build, stdenv's unpackPhase copies the source tree
    # from the Nix store into a writable temporary directory.  CMake compiles
    # from that temporary copy, so .gcno files embed paths like
    # /build/source/include/fil/... rather than the Nix store path in ${src}.
    # We therefore read CMAKE_HOME_DIRECTORY from CMakeCache.txt (always
    # present in the CMake build directory, which is the CWD during postCheck)
    # to get the actual compile-time source root.  Fall back to the parent
    # directory, which is also the source root in all Nix CMake builds where
    # the build dir is a subdirectory of the source.
    if sourceDir=$(grep "^CMAKE_HOME_DIRECTORY:INTERNAL=" CMakeCache.txt 2>/dev/null | cut -d= -f2) \
       && [ -n "$sourceDir" ]; then
      echo "Coverage: source directory from CMakeCache.txt: $sourceDir"
    else
      sourceDir="$(pwd)/.."
      echo "Coverage: CMakeCache.txt lookup failed, using fallback: $sourceDir"
    fi

    # --root "$sourceDir"             : path gcovr uses to resolve source-file paths
    #                                   embedded in .gcno files; also makes output paths
    #                                   repo-relative (e.g. include/fil/algorithm/contains.hh)
    #                                   which is required for Codacy to match them.
    # --object-directory "."          : search the CMake build dir (CWD in postCheck)
    #                                   for .gcda/.gcno files; required because --root is
    #                                   now the source dir, not the build dir.
    # --exclude "$sourceDir/tests/.*" : keep test drivers out of the library report.
    gcovr \
          --root "$sourceDir" \
          --object-directory "." \
          --exclude "$sourceDir/tests/.*" \
          --gcov-executable gcov \
          --exclude-unreachable-branches \
          --xml \
          --xml-pretty \
          --output "$out/coverage/cobertura.xml"

  '';

  postInstall = "";

  meta = with lib; {
    description = "Header-only C++ Utility library";
    homepage = "https://github.com/FreeYourSoul/FiL";
    license = licenses.agpl3Plus;
    platforms = platforms.all;
    maintainers = with maintainers; [
      freeyoursoul
    ];
  };
}
