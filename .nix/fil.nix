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
    echo "Generating coverage report... ${src}"

    mkdir -p $out/coverage

    # Generate gcovr Cobertura XML report.
    # --root "${src}"          : source files live in the Nix store source path; paths in
    #                            the report become relative to the repo root (e.g.
    #                            include/fil/algorithm/contains.hh) which is what Codacy
    #                            needs to map them back to the repository.
    # --object-directory "."   : .gcda/.gcno files are produced in the CMake build
    #                            directory (current directory), not in ${src}.  This is
    #                            critical when --root is not the build directory.
    # --exclude "${src}/tests/.*" : exclude the test driver files; we want coverage of
    #                            the library headers, not the test code itself.
    # No --exclude "/nix/store/.*" : that rule was accidentally excluding ALL source files
    #                            because every file in a Nix build lives under /nix/store.
    gcovr \
          --root "${src}" \
          --object-directory "." \
          --exclude "${src}/tests/.*" \
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
