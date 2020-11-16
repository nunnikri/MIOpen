
def rocmnode(name) {
    def node_name = 'rocmtest'
    if(name == 'fiji') {
        node_name = 'rocmtest && fiji';
    } else if(name == 'vega') {
        node_name = 'rocmtest && vega';
    } else if(name == 'vega10') {
        node_name = 'rocmtest && vega10';
    } else if(name == 'vega20') {
        node_name = 'rocmtest && vega20';
    } else if(name == 'gfx908') {
        node_name = 'gfx908';
    } else {
        node_name = name
    }
    return node_name
}



def cmake_build(compiler, flags, env4make, extradebugflags, prefixpath, cmd="", testflags=""){

    def workspace_dir = pwd()
    def vcache = "/var/jenkins/.cache/miopen/vcache"
    def archive = (flags == '-DCMAKE_BUILD_TYPE=release')
    def config_targets = "check doc MIOpenDriver"
    def test_flags = "--disable-verification-cache " + testflags
    def debug_flags = "-g ${extradebugflags} -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=undefined"
    def compilerpath = ""
    def configargs = ""
    if (prefixpath == "/usr/local")
        compilerpath = compiler;
    else
    {
        compilerpath = prefixpath + "/bin/" + compiler
        configargs = "-DCMAKE_PREFIX_PATH=${prefixpath}"
    }

    if (archive == true) {
        config_targets = "package"
    }

    def postcmd = ""
    precmd = """
            echo \$HSA_ENABLE_SDMA
            ulimit -c unlimited
            rm -rf build
            mkdir build
            cd build
            CXX=${compilerpath} CXXFLAGS='-Werror' cmake ${configargs} -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_FLAGS='${test_flags}' -DCMAKE_CXX_FLAGS_DEBUG='${debug_flags}' ${flags} .. 
        """
    if (cmd == "")
    {
        postcmd = """
            ${precmd}
            MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_XDLOPS=1 CTEST_PARALLEL_LEVEL=4 MIOPEN_VERIFY_CACHE_PATH=${vcache} MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 ${env4make} dumb-init make -j\$(nproc) ${config_targets}
        """

    }
    else
    {
        postcmd = """
            ${precmd}
            ${cmd}
        """
    }

    echo postcmd
    sh postcmd

    // Only archive from master or develop
    if (archive == true && (env.BRANCH_NAME == "develop" || env.BRANCH_NAME == "master")) {
        archiveArtifacts artifacts: "build/*.deb", allowEmptyArchive: true, fingerprint: true
    }
}

def buildJob(Map conf, compiler){

        env.HSA_ENABLE_SDMA=0 
        env.CODECOV_TOKEN="aec031be-7673-43b5-9840-d8fb71a2354e"
        checkout scm
        def prefixpath = conf.get("prefixpath", "/usr/local")
        def flags = conf.get("flags", "")
        def env4make = conf.get("env4make", "")
        def image = conf.get("image", "miopen")
        def cmd = conf.get("cmd", "")
        def gpu_arch = conf.get("gpu_arch", "all")
        def codecov = conf.get("codecov", false)
        def dockerOpts="--device=/dev/kfd --device=/dev/dri --group-add video --cap-add=SYS_PTRACE --security-opt seccomp=unconfined"
        def dockerArgs = "--build-arg PREFIX=${prefixpath} --build-arg GPU_ARCH='${gpu_arch}' "
        def extradebugflags = ""
        def variant = env.STAGE_NAME
        if (codecov) {
            extradebugflags = "-fprofile-arcs -ftest-coverage"
        }
        def retimage
        gitStatusWrapper(credentialsId: '7126e5fe-eb51-4576-b52b-9aaf1de8f0fd', gitHubContext: "Jenkins - ${variant}", account: 'ROCmSoftwarePlatform', repo: 'MIOpen') {
            try {
                retimage = docker.build("${image}", dockerArgs + '.')
                withDockerContainer(image: image, args: dockerOpts) {
                    timeout(time: 5, unit: 'MINUTES')
                    {
                        sh 'PATH="/opt/rocm/opencl/bin/x86_64/:$PATH" clinfo'
                    }
                }
            } catch(Exception ex) {
                retimage = docker.build("${image}", dockerArgs + "--no-cache .")
                withDockerContainer(image: image, args: dockerOpts) {
                    timeout(time: 5, unit: 'MINUTES')
                    {
                        sh 'PATH="/opt/rocm/opencl/bin/x86_64/:$PATH" clinfo'
                    }
                }
            }

            withDockerContainer(image: image, args: dockerOpts + ' -v=/var/jenkins/:/var/jenkins') {
                timeout(time: 5, unit: 'HOURS')
                {
                    if(cmd == ""){
                        cmake_build(compiler, flags, env4make, extradebugflags, prefixpath)
                    }else{
                        sh cmd
                    }
                    if (codecov) {
                        sh '''
                            cd build
                            lcov --directory . --capture --output-file $(pwd)/coverage.info
                            lcov --remove $(pwd)/coverage.info '/usr/*' --output-file $(pwd)/coverage.info
                            lcov --list $(pwd)/coverage.info
                            curl -s https://codecov.io/bash | bash
                            echo "Uploaded"
                        '''
                    }
                }
            }
        }
        return retimage
}

def buildHipClangJob(compiler, flags, env4make, image, prefixpath="/opt/rocm", cmd = "", gpu_arch="all", miot_ver="default"){

        env.HSA_ENABLE_SDMA=0 
        checkout scm
        def dockerOpts="--device=/dev/kfd --device=/dev/dri --group-add video --cap-add=SYS_PTRACE --security-opt seccomp=unconfined"
        def dockerArgs = "--build-arg PREFIX=${prefixpath} --build-arg GPU_ARCH='${gpu_arch}' --build-arg MIOTENSILE_VER='${miot_ver}' -f hip-clang.docker "
        def variant = env.STAGE_NAME
        def retimage
        gitStatusWrapper(credentialsId: '7126e5fe-eb51-4576-b52b-9aaf1de8f0fd', gitHubContext: "Jenkins - ${variant}", account: 'ROCmSoftwarePlatform', repo: 'MIOpen') {
            try {
                retimage = docker.build("${image}", dockerArgs + '.')
                withDockerContainer(image: image, args: dockerOpts) {
                    timeout(time: 5, unit: 'MINUTES')
                    {
                        sh 'PATH="/opt/rocm/opencl/bin:/opt/rocm/opencl/bin/x86_64:$PATH" clinfo'
                    }
                }
            } catch(Exception ex) {
                retimage = docker.build("${image}", dockerArgs + "--no-cache .")
                withDockerContainer(image: image, args: dockerOpts) {
                    timeout(time: 5, unit: 'MINUTES')
                    {
                        sh 'PATH="/opt/rocm/opencl/bin:/opt/rocm/opencl/bin/x86_64:$PATH" clinfo'
                    }
                }
            }

            withDockerContainer(image: image, args: dockerOpts + ' -v=/var/jenkins/:/var/jenkins') {
                timeout(time: 5, unit: 'HOURS')
                {
                    if(cmd == ""){
                        cmake_build(compiler, flags, env4make, prefixpath)
                    }else{
                        sh cmd
                    }
                }
            }
        }
        return retimage
}



def buildCommandJob(cmd, image, prefixpath=""){

        checkout scm
        def dockerOpts="--device=/dev/kfd --device=/dev/dri --group-add video --cap-add=SYS_PTRACE --security-opt seccomp=unconfined"
        def dockerArgs = "--build-arg PREFIX=${prefixpath} "
	def variant = env.STAGE_NAME
        if(prefixpath == "")
        {
            dockerArgs = ""
        }
       
        gitStatusWrapper(credentialsId: '7126e5fe-eb51-4576-b52b-9aaf1de8f0fd', gitHubContext: "Jenkins - ${variant}", account: 'ROCmSoftwarePlatform', repo: 'MIOpen') {
            try {
                retimage = docker.build("${image}", dockerArgs + '.')
                withDockerContainer(image: image, args: dockerOpts + ' -v=/var/jenkins/:/var/jenkins') {
                    timeout(time: 5, unit: 'HOURS')
                    {
                        sh cmd
                    }
                }
            } catch(Exception ex) {
                retimage = docker.build("${image}", dockerArgs + "--no-cache .")
                withDockerContainer(image: image, args: dockerOpts + ' -v=/var/jenkins/:/var/jenkins') {
                    timeout(time: 5, unit: 'HOURS')
                    {
                        sh cmd
                    }
                }
            }
        }
}


pipeline {
    agent none 
    options {
        parallelsAlwaysFailFast()
    }
    environment{
        image = "miopen"
    }
    stages{
        // Run all static analysis tests

        stage("Static checks"){
            parallel{
                stage('Clang Tidy') {
                    agent{  label rocmnode("rocmtest") }
                    environment{
                        cmd = "rm -rf build; mkdir build; cd build; CXX='clang++-3.8' cmake -DBUILD_DEV=On ..; make -j\$(nproc) -k analyze;"
                    }
                    steps{
                        buildCommandJob(cmd, image)
                    }
                }

                stage('Clang Format') {
                    agent{ label rocmnode("rocmtest") }
                    environment{
                        cmd = "find . -iname \'*.h\' \
                                -o -iname \'*.hpp\' \
                                -o -iname \'*.cpp\' \
                                -o -iname \'*.h.in\' \
                                -o -iname \'*.hpp.in\' \
                                -o -iname \'*.cpp.in\' \
                                -o -iname \'*.cl\' \
                                | grep -v 'build/' \
                                | xargs -n 1 -P 1 -I{} -t sh -c \'clang-format-3.8 -style=file {} | diff - {}\'"
                    }
                    steps{
                        buildCommandJob(cmd, image)
                    }
                }

                stage('Hip Tidy') {
                    agent{ label rocmnode("rocmtest") }
                    environment{
                        cmd = "rm -rf build; mkdir build; cd build; CXX=/usr/local/bin/hcc cmake -DBUILD_DEV=On ..; make -j\$(nproc) -k analyze;"
                    }
                    steps{
                        buildCommandJob(cmd, image)
                    }
                }
            }
        }
        
        // Run quick fp32 tests
        stage("Fast full precision"){
            parallel{
               stage('Clang Debug') {
                    agent{ label rocmnode("vega") }
                    steps{
                        buildJob('clang++-3.8', flags: '-DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug', gpu_arch: "gfx900;gfx906")
                    }
                }

                stage('Clang Release') {
                    agent{ label rocmnode("vega") }
                    steps{
                        buildJob('clang++-3.8', flags: '-DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release', gpu_arch: "gfx900;gfx906")
                    }
                }

                stage('Hip debug') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """

                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx900;gfx906")
                    }
                }

                stage('Hip conv2d debug quick') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            make -j\$(nproc) test_conv2d
                            bin/test_conv2d --disable-verification-cache
                        """

                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx900;gfx906")
                    }
                }

                stage('Hip release') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """

                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx900;gfx906")
                    }
                }

                stage('Hip gfx908 debug') {
                    agent{ label rocmnode("gfx908") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DMIOPEN_TEST_GFX908=On -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx908")
                    }
                }
            }
        }

        // Misc tests
        stage("Aux tests"){
            parallel{
                stage('Hip clang debug COMGR') {
                    // WORKAROUND for COMGR Vega10 testing problem. Should be "vega".
                    agent{ label rocmnode("vega20") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DMIOPEN_USE_COMGR=On -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            CTEST_PARALLEL_LEVEL=2 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """

                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "MIOPEN_LOG_LEVEL=5 MIOPEN_COMPILE_PARALLEL_LEVEL=1",  image+'-hip-clang', "/usr/local", cmd, "gfx906")
                    }
                }
                stage('Hip clang Embed Build') {
                    agent{ label rocmnode("vega20") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DMIOPEN_EMBED_DB="gfx906_60;gfx906_64" -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            make -j\$(nproc) check
                        """

                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "MIOPEN_LOG_LEVEL=5 MIOPEN_COMPILE_PARALLEL_LEVEL=1",  image+'-hip-clang', "/usr/local", cmd, "gfx906")
                    }
                }

                stage('Hip Static Release') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DBUILD_EMBED_BUILD=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """

                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "",  image+'-hip-clang', "/usr/local", cmd, "gfx900;gfx906")
                    }
                }

                stage('Hip Normal Find Mode Release') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_GPU_SYNC=On .. 
                            make -j test_conv2d
                            MIOPEN_FIND_MODE=1 CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 bin/test_conv2d --disable-verification-cache
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "",  image+'-hip-clang', "/usr/local", cmd, "gfx900;gfx906")
                    }
                }

                stage('Hip Fast Find Mode Release') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_GPU_SYNC=On .. 
                            make -j test_conv2d
                            MIOPEN_FIND_MODE=2 CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 bin/test_conv2d --disable-verification-cache
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "",  image+'-hip-clang', "/usr/local", cmd, "gfx900;gfx906")
                    }
                }

                stage('Hip Release on /usr/local') {
                    agent{ label rocmnode("vega") }
                    steps{
                        buildJob('hcc', flags: '-DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release', gpu_arch: "gfx900;gfx906")
                    }
                }

            }
        }

        // Run fp16, bfp16, and int8 quick tests
        stage("Fast low precision"){
            parallel{
                stage('Half Hip Release') {
                    agent{ label rocmnode("vega20") }
                    steps{
                        buildJob('hcc', flags: '-DMIOPEN_TEST_HALF=On -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release', image: image+"rocm", prefixpath: '/opt/rocm', gpu_arch: "gfx906")
                    }
                }
                
                stage('Bfloat16 gfx908 HCC Debug') {
                    agent{ label rocmnode("gfx908") }   
                    steps{
                        buildJob('hcc', flags: '-DMIOPEN_TEST_BFLOAT16=On -DMIOPEN_TEST_GFX908=On -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug', image: image+"rocm", prefixpath: '/opt/rocm', gpu_arch: "gfx908")
                    }
                }
                stage('Half gfx908 HCC Debug') {
                    agent{ label rocmnode("gfx908") }   
                    steps{
                        buildJob('hcc', flags: '-DMIOPEN_TEST_BFLOAT16=On -DMIOPEN_TEST_GFX908=On -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug', image: image+"rocm", prefixpath: '/opt/rocm', gpu_arch: "gfx908")
                    }
                }
            }
        }

        stage("Long Tests I"){
            parallel{
                stage('Int8 conv2d Release conv2d') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DMIOPEN_TEST_INT8=On -DMIOPEN_USE_COMGR=Off -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=Release -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_ALL=On -DMIOPEN_TEST_LIMIT=2 -DMIOPEN_TEST_FLAGS="--disable-verification-cache" .. 
                            make -j test_conv2d
                            MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 bin/test_conv2d --limit 3 --disable-verification-cache
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx906")
                    }
                }

                stage('GCC Release conv2d') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/usr/bin/g++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_HIP_COMPILER=/opt/rocm/llvm/bin/clang++ -DMIOPEN_BACKEND=OpenCL -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_ALL=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            make -j test_conv2d
                            CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 bin/test_conv2d --limit 3 --disable-verification-cache
                        """
                    }
		    steps{
		        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx900;gfx906")
		    }
		}

                stage('GCC codecov') {
                    agent{ label rocmnode("vega") }
                    steps{
                        buildJob('g++-5', flags: '-DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug', codecov: true, gpu_arch: "gfx900;gfx906")
                    }
                }
                
                stage('Bfloat16 gfx908 Hip Release All Subset') {
                    agent{ label rocmnode("gfx908") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DMIOPEN_TEST_BFLOAT16=On -DMIOPEN_TEST_GFX908=On -DMIOPEN_USE_COMGR=Off -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=Release -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_ALL=On -DMIOPEN_TEST_LIMIT=2 -DMIOPEN_TEST_FLAGS="--disable-verification-cache" .. 
                            CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "",  image+'-hip-clang', "/usr/local", cmd, "gfx908")
                    }
                }
            }
        }

        stage("Long Tests II"){
            parallel{
                
                stage('Hip Clang Release All') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DMIOPEN_SKIP_CONV2D=On -DMIOPEN_SKIP_CONV3D=On -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=debug -DMIOPEN_GPU_SYNC=On -DMIOPEN_USE_COMGR=Off -DMIOPEN_TEST_ALL=On -DMIOPEN_TEST_LIMIT=2 -DMIOPEN_TEST_FLAGS="--disable-verification-cache" .. 
                            MIOPEN_ENABLE_LOGGING_CMD=1 CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx900;gfx906")
                    }
                }

                stage('FP32 gfx908 Hip Release All subset') {
                    agent{ label rocmnode("gfx908") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_TEST_GFX908=On -DMIOPEN_TEST_LIMIT=2 -DMIOPEN_GPU_SYNC=On -DMIOPEN_USE_COMGR=Off -DMIOPEN_TEST_ALL=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx908")
                    }
                }       
            }
        }

        stage("Long Tests III"){
            parallel{
                stage('Half Hip Clang Release All') {
                    agent{ label rocmnode("vega20") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_TEST_HALF=On -DMIOPEN_GPU_SYNC=On -DMIOPEN_USE_COMGR=Off -DMIOPEN_TEST_LIMIT=2 -DMIOPEN_TEST_ALL=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """

                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "",  image+'-hip-clang', "/usr/local", cmd, "gfx908")
                    }
                }

                stage('Half gfx908 Hip Release All Subset') {
                    agent{ label rocmnode("gfx908") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_TEST_GFX908=On -DMIOPEN_USE_COMGR=Off -DMIOPEN_TEST_HALF=On -DMIOPEN_TEST_LIMIT=2 -DMIOPEN_GPU_SYNC=On -DMIOPEN_TEST_ALL=On -DMIOPEN_TEST_FLAGS=--disable-verification-cache .. 
                            CTEST_PARALLEL_LEVEL=4 MIOPEN_DEBUG_IMPLICIT_GEMM_NON_XDLOPS_INLINE_ASM=0 MIOPEN_CONV_PRECISE_ROCBLAS_TIMING=0 make -j\$(nproc) check
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "",  image+'-hip-clang', "/usr/local", cmd, "gfx908")
                    }
                }
            }
        }

        stage("Long Tests IV"){
            parallel{
                stage('Hip Clang conv3d') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_GPU_SYNC=Off -DMIOPEN_USE_COMGR=Off -DMIOPEN_TEST_ALL=On -DMIOPEN_TEST_LIMIT=2 .. 
                            make -j test_conv3d
                            bin/test_conv3d --all --limit=2 --disable-verification-cache
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx906")
                    }
                }

                stage('Hip Clang conv2d') {
                    agent{ label rocmnode("vega") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DBUILD_DEV=On -DMIOPEN_SKIP_ALL_BUT_CONV2D=On -DCMAKE_BUILD_TYPE=release -DMIOPEN_GPU_SYNC=On -DMIOPEN_USE_COMGR=Off -DMIOPEN_TEST_ALL=On -DMIOPEN_TEST_LIMIT=2 -DMIOPEN_TEST_FLAGS="--disable-verification-cache" .. 
                            make -j\$(nproc) check
                        """
                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "gfx906")
                    }
                }
                
            }
        }

       // Run package building
        stage("Packages"){
            parallel {
                stage('GCC OpenCL Release package') {
                    agent{ label rocmnode("rocmtest") }
                    steps{
                        buildJob('g++-5', flags: '-DCMAKE_BUILD_TYPE=release', gpu_arch: "all")
                    }
                }
 
                stage('Hip Clang Release package') {
                    agent{ label rocmnode("rocmtest") }
                    environment{
                        cmd = """
                            ulimit -c unlimited
                            rm -rf build
                            mkdir build
                            cd build
                            CXX=/opt/rocm/llvm/bin/clang++ cmake -DCMAKE_BUILD_TYPE=release .. 
                            make -j\$(nproc) package
                        """

                    }
                    steps{
                        buildHipClangJob('/opt/rocm/llvm/bin/clang++', '', "", image+'-hip-clang', "/usr/local", cmd, "all")
                    }
                }
            }
        }
    }    
}

