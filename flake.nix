{
  description = "Fil : A C++ generic library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };


  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
    let
        pkgs = nixpkgs.legacyPackages.${system};

    in rec {
        rocksdb = pkgs.stdenv.mkDerivation rec {
              pname = "rocksdb";
              version = "7.10.2";

              src = pkgs.fetchFromGitHub {
                owner = "facebook";
                repo = "rocksdb";
                rev = "v${version}";
                sha256 = "sha256-Np3HPTZYzyoPOKL0xgsLzcvOkceFiEQd+1nyGbg4BHo=";
              };

              nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];

              propagatedBuildInputs = [ pkgs.bzip2 pkgs.lz4 pkgs.snappy pkgs.zlib pkgs.zstd ];

              NIX_CFLAGS_COMPILE = pkgs.lib.optionalString pkgs.stdenv.cc.isGNU "-Wno-error=deprecated-copy -Wno-error=pessimizing-move"
                + pkgs.lib.optionalString pkgs.stdenv.cc.isClang "-Wno-error=unused-private-field -faligned-allocation";

              cmakeFlags = [
                "-DPORTABLE=1"
                "-DWITH_JEMALLOC=0"
                "-DWITH_JNI=0"
                "-DWITH_BENCHMARK_TOOLS=0"
                "-DWITH_TESTS=1"
                "-DWITH_TOOLS=0"
                "-DWITH_BZ2=1"
                "-DWITH_LZ4=1"
                "-DWITH_SNAPPY=1"
                "-DWITH_ZLIB=1"
                "-DWITH_ZSTD=1"
                "-DWITH_GFLAGS=0"
                "-DUSE_RTTI=1"
                "-DROCKSDB_INSTALL_ON_WINDOWS=YES" # harmless elsewhere
                "-DROCKSDB_BUILD_SHARED=0"
                (pkgs.lib.optional
                    (pkgs.stdenv.hostPlatform.isx86 && pkgs.stdenv.hostPlatform.isLinux)
                    "-DFORCE_SSE42=1")
                "-DFAIL_ON_WARNINGS=${if pkgs.stdenv.hostPlatform.isMinGW then "NO" else "YES"}"
              ];

              # otherwise "cc1: error: -Wformat-security ignored without -Wformat [-Werror=format-security]"
              hardeningDisable = pkgs.lib.optional pkgs.stdenv.hostPlatform.isWindows "format";

              # Old version doesn't ship the .pc file, new version puts wrong paths in there.
              postFixup = ''
                if [ -f "$out"/lib/pkgconfig/rocksdb.pc ]; then
                  substituteInPlace "$out"/lib/pkgconfig/rocksdb.pc \
                    --replace '="''${prefix}//' '="/'
                fi
              '';

              meta = with pkgs.lib; {
                homepage = "https://rocksdb.org";
                description = "A library that provides an embeddable, persistent key-value store for fast storage";
                changelog = "https://github.com/facebook/rocksdb/raw/v${version}/HISTORY.md";
                license = licenses.asl20;
                platforms = platforms.all;
              };
          };

         fil = pkgs.stdenv.mkDerivation {
             pname = "fil";
             version = "1.0.1";

             src = self;

             buildInputs = [ pkgs.cmake rocksdb pkgs.catch2 pkgs.fmt ];
        };


        defaultPackage = fil;

    });

}





