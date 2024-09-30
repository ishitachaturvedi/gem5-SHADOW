# Copied from https://github.com/darchr/novoverse/blob/main/components/processors/grace/gracecore/grace_o3_cpu.py

from m5.objects import *

# from m5.objects import (
#     ArmO3CPU,
#     FUDesc,
#     OpDesc,
#     FUPool,
# )

# from m5.objects.FUPool import *

#from m5.objects.BranchPredictor import BiModeBP, SimpleBTB


class O3_ARM_Grace_FP_Vec_0(FUDesc):
    """
    This class refers to FP/ASIMD 0/1 (symbol V in (2) table 3)
    Copied from Neoverse V1 optimization guide,
    latency taken for specific instruction in brackets
    """

    # ASIMD arithmetic basis (add & sub)
    opList = [
        # FP arith (fadd)-3.12
        OpDesc(opClass="FloatAdd", opLat=2),
        # FP compare (fccmpe)-3.12
        OpDesc(opClass="FloatCmp", opLat=2),
        # Fp (vfma)-3.12
        OpDesc(opClass="FloatMultAcc", opLat=4),
        # Fp convert (vcvt)-3.13
        OpDesc(opClass="FloatCvt", opLat=3),
        # Vec arith basis (add)-3.16
        OpDesc(opClass="SimdAdd", opLat=2),
        # Vec logical (and)  -3.16
        OpDesc(opClass="SimdAlu", opLat=2),
        # Vec compare (cmeq) -3.16
        OpDesc(opClass="SimdCmp", opLat=2),
        # miscelleaneaus
        OpDesc(opClass="FloatMisc", opLat=3),
        # (vadd) -3.17
        OpDesc(opClass="SimdFloatAdd", opLat=2),
        # Vec (vmla)-3.17
        OpDesc(opClass="SimdFloatMultAcc", opLat=4),
        # Vec FP comapre (fcmgt)-3.17
        OpDesc(opClass="SimdFloatCmp", opLat=2),
        # SVE FP Red f64 (fmaxv)
        OpDesc(opClass="SimdFloatReduceCmp", opLat=3),
        # Fp multiply (fmul)-3.12
        OpDesc(opClass="FloatMult", opLat=3),
        # Fp multiply (fmul)-3.22
        OpDesc(opClass="SimdAes", opLat=2),
        # Fp multiply (fmul)-3.22
        OpDesc(opClass="SimdAesMix", opLat=2),
        # Vec integer multiply(mul)-3.16
        OpDesc(opClass="SimdMult", opLat=4),
        # Vec move, immed (vmov) -3.16
        OpDesc(opClass="SimdMisc", opLat=2),
        # SVE reduction (saddv)-3.16
        OpDesc(opClass="SimdReduceAdd", opLat=2),
        # SVE reduction, logical (andv)
        OpDesc(opClass="SimdReduceAlu", opLat=3),
        # SVE FP associative add (fadda)
        OpDesc(opClass="SimdFloatReduceAdd", opLat=2, pipelined=True),
        # misc insts (vneg)-3.17
        OpDesc(opClass="SimdFloatMisc", opLat=2),
        # PREDALU has instructions in V group ( trn1 and uzp )
        OpDesc(opClass="SimdPredAlu", opLat=2),
        # V02 and V01
        # Fp divide (vdiv)- average latency-3.13
        OpDesc(opClass="FloatDiv", opLat=11, pipelined=True),
        # Vec FP divide f64 (fdiv)- we take average latency-3.17
        OpDesc(opClass="SimdFloatDiv", opLat=11, pipelined=True),
        # Fp square root D-form (fsqrt)- average latency-3.13
        OpDesc(opClass="FloatSqrt", opLat=12, pipelined=True),
        # Vec reciprocal estimate (vrsqrte)
        OpDesc(opClass="SimdSqrt", opLat=9),
        # Vec FP square root f64 (vsqrt)- we take average latency-3.17
        OpDesc(opClass="SimdFloatSqrt", opLat=12, pipelined=True),
        # Vec multiply accumulate, D-form (mla) -3.16
        OpDesc(opClass="SimdMultAcc", opLat=4),
        # Vec FP convert to FP 64b (scvtf)-3.17
        OpDesc(opClass="SimdCvt", opLat=3),
        # V0
        # Crypto SHA1 hash acceleration op(sha1h) - 2 latency -3.22
        OpDesc(opClass="SimdSha1Hash2", opLat=2),
        # Crypto SHA1 hash acceleration ops (SHA1M)-3.22
        OpDesc(opClass="SimdSha1Hash", opLat=4),
        # Crypto SHA1 schedule acceleration ops (SHA1SU0)-3.22
        OpDesc(opClass="SimdShaSigma3", opLat=2),
        # Crypto SHA256 hash acceleration ops (SHA256H) -3.22
        OpDesc(opClass="SimdSha256Hash", opLat=4),
        # Crypto SHA256 hash acceleration ops (SHA256H2)-3.22
        OpDesc(opClass="SimdSha256Hash2", opLat=4),
        # Crypto SHA256 schedule acceleration ops(sha256su0)-3.22
        OpDesc(opClass="SimdShaSigma2", opLat=2),
        # Crypto SHA256 schedule acceleration ops(sha256su1)-3.22
        OpDesc(opClass="SimdShaSigma3", opLat=2),
    ]

    count = 1


class O3_ARM_Grace_FP_Vec_1(FUDesc):
    opList = [
        # Common V
        # FP arith (fadd)-3.12
        OpDesc(opClass="FloatAdd", opLat=2),
        # FP compare (fccmpe)-3.12
        OpDesc(opClass="FloatCmp", opLat=2),
        # Fp (vfma)-3.12
        OpDesc(opClass="FloatMultAcc", opLat=4),
        # Fp convert (vcvt)-3.13
        OpDesc(opClass="FloatCvt", opLat=3),
        # Vec arith basis (add)-3.16
        OpDesc(opClass="SimdAdd", opLat=2),
        # Vec logical (and)  -3.16
        OpDesc(opClass="SimdAlu", opLat=2),
        # Vec compare (cmeq) -3.16
        OpDesc(opClass="SimdCmp", opLat=2),
        # miscelleaneaus
        OpDesc(opClass="FloatMisc", opLat=3),
        # (vadd) -3.17
        OpDesc(opClass="SimdFloatAdd", opLat=2),
        # Vec (vmla)-3.17
        OpDesc(opClass="SimdFloatMultAcc", opLat=4),
        # Vec FP comapre (fcmgt)-3.17
        OpDesc(opClass="SimdFloatCmp", opLat=2),
        # SVE FP Red f64 (fmaxv)
        OpDesc(opClass="SimdFloatReduceCmp", opLat=3),
        # Fp multiply (fmul)-3.12
        OpDesc(opClass="FloatMult", opLat=3),
        # Fp multiply (fmul)-3.22
        OpDesc(opClass="SimdAes", opLat=2),
        # Fp multiply (fmul)-3.22
        OpDesc(opClass="SimdAesMix", opLat=2),
        # Vec integer multiply(mul)-3.16
        OpDesc(opClass="SimdMult", opLat=4),
        # Vec move, immed (vmov) -3.16
        OpDesc(opClass="SimdMisc", opLat=2),
        # SVE reduction (saddv)-3.16
        OpDesc(opClass="SimdReduceAdd", opLat=2),
        # SVE reduction, logical (andv)
        OpDesc(opClass="SimdReduceAlu", opLat=3),
        # SVE FP associative add (fadda)
        OpDesc(opClass="SimdFloatReduceAdd", opLat=2, pipelined=True),
        # misc insts (vneg)-3.17
        OpDesc(opClass="SimdFloatMisc", opLat=2),
        # PREDALU has instructions in V group ( trn1 and uzp )
        OpDesc(opClass="SimdPredAlu", opLat=2),
        # V1
        # Vec FP convert to FP 64b (scvtf)-3.17
        OpDesc(opClass="SimdCvt", opLat=3),
        # PREDALU has instructions in V1 group ( COMPACT), EOR
        OpDesc(opClass="SimdPredAlu", opLat=3),
        # V13
        # SVE reduction, arith, S form (smaxv)-3.16
        OpDesc(opClass="SimdReduceCmp", opLat=2),
        # Vec absolute diff accum (vaba) -3.16
        OpDesc(opClass="SimdAddAcc", opLat=4),
        # Vec shift by immed, (shl)-3.16
        OpDesc(opClass="SimdShift", opLat=2),
        # Vec shift accumulate (vsra)-3.16
        OpDesc(opClass="SimdShiftAcc", opLat=4),
    ]
    count = 1


class O3_ARM_Grace_FP_Vec_2(FUDesc):
    opList = [
        # FP arith (fadd)-3.12
        OpDesc(opClass="FloatAdd", opLat=2),
        # FP compare (fccmpe)-3.12
        OpDesc(opClass="FloatCmp", opLat=2),
        # Fp (vfma)-3.12
        OpDesc(opClass="FloatMultAcc", opLat=4),
        # Fp convert (vcvt)-3.13
        OpDesc(opClass="FloatCvt", opLat=3),
        # Vec arith basis (add)-3.16
        OpDesc(opClass="SimdAdd", opLat=2),
        # Vec logical (and)  -3.16
        OpDesc(opClass="SimdAlu", opLat=2),
        # Vec compare (cmeq) -3.16
        OpDesc(opClass="SimdCmp", opLat=2),
        # miscelleaneaus
        OpDesc(opClass="FloatMisc", opLat=3),
        # (vadd) -3.17
        OpDesc(opClass="SimdFloatAdd", opLat=2),
        # Vec (vmla)-3.17
        OpDesc(opClass="SimdFloatMultAcc", opLat=4),
        # Vec FP comapre (fcmgt)-3.17
        OpDesc(opClass="SimdFloatCmp", opLat=2),
        # SVE FP Red f64 (fmaxv)
        OpDesc(opClass="SimdFloatReduceCmp", opLat=3),
        # Fp multiply (fmul)-3.12
        OpDesc(opClass="FloatMult", opLat=3),
        # Fp multiply (fmul)-3.22
        OpDesc(opClass="SimdAes", opLat=2),
        # Fp multiply (fmul)-3.22
        OpDesc(opClass="SimdAesMix", opLat=2),
        # Vec integer multiply(mul)-3.16
        OpDesc(opClass="SimdMult", opLat=4),
        # Vec move, immed (vmov) -3.16
        OpDesc(opClass="SimdMisc", opLat=2),
        # SVE reduction (saddv)-3.16
        OpDesc(opClass="SimdReduceAdd", opLat=2),
        # SVE reduction, logical (andv)
        OpDesc(opClass="SimdReduceAlu", opLat=3),
        # SVE FP associative add (fadda)
        OpDesc(opClass="SimdFloatReduceAdd", opLat=2, pipelined=True),
        # misc insts (vneg)-3.17
        OpDesc(opClass="SimdFloatMisc", opLat=2),
        # PREDALU has instructions in V group ( trn1 and uzp )
        OpDesc(opClass="SimdPredAlu", opLat=2),
        # Fp divide (vdiv)- average latency-3.13
        OpDesc(opClass="FloatDiv", opLat=11, pipelined=True),
        # Vec FP divide f64 (fdiv)- we take average latency-3.17
        OpDesc(opClass="SimdFloatDiv", opLat=11, pipelined=True),
        # Fp square root D-form (fsqrt)- average latency-3.13
        OpDesc(opClass="FloatSqrt", opLat=12, pipelined=True),
        # Vec FP convert to FP 64b (scvtf)-3.17
        OpDesc(opClass="SimdCvt", opLat=3),
        # Vec multiply accumulate, D-form (mla) -3.16
        OpDesc(opClass="SimdMultAcc", opLat=4),
        # Vec reciprocal estimate (vrsqrte)
        OpDesc(opClass="SimdSqrt", opLat=9),
        # Vec FP square root f64 (vsqrt)- we take average latency-3.17
        OpDesc(opClass="SimdFloatSqrt", opLat=12, pipelined=True),
    ]
    count = 1


class O3_ARM_Grace_FP_Vec_3(FUDesc):
    opList = [
        # Common V
        # FP arith (fadd)-3.12
        OpDesc(opClass="FloatAdd", opLat=2),
        # FP compare (fccmpe)-3.12
        OpDesc(opClass="FloatCmp", opLat=2),
        # Fp (vfma)-3.12
        OpDesc(opClass="FloatMultAcc", opLat=4),
        # Fp convert (vcvt)-3.13
        OpDesc(opClass="FloatCvt", opLat=3),
        # Vec arith basis (add)-3.16
        OpDesc(opClass="SimdAdd", opLat=2),
        # Vec logical (and)  -3.16
        OpDesc(opClass="SimdAlu", opLat=2),
        # Vec compare (cmeq) -3.16
        OpDesc(opClass="SimdCmp", opLat=2),
        # miscelleaneaus
        OpDesc(opClass="FloatMisc", opLat=3),
        # (vadd) -3.17
        OpDesc(opClass="SimdFloatAdd", opLat=2),
        # Vec (vmla)-3.17
        OpDesc(opClass="SimdFloatMultAcc", opLat=4),
        # Vec FP comapre (fcmgt)-3.17
        OpDesc(opClass="SimdFloatCmp", opLat=2),
        # SVE FP Red f64 (fmaxv)
        OpDesc(opClass="SimdFloatReduceCmp", opLat=3),
        # Fp multiply (fmul)-3.12
        OpDesc(opClass="FloatMult", opLat=3),
        # Fp multiply (fmul)-3.22
        OpDesc(opClass="SimdAes", opLat=2),
        # Fp multiply (fmul)-3.22
        OpDesc(opClass="SimdAesMix", opLat=2),
        # Vec integer multiply(mul)-3.16
        OpDesc(opClass="SimdMult", opLat=4),
        # Vec move, immed (vmov) -3.16
        OpDesc(opClass="SimdMisc", opLat=2),
        # SVE reduction (saddv)-3.16
        OpDesc(opClass="SimdReduceAdd", opLat=2),
        # SVE reduction, logical (andv)
        OpDesc(opClass="SimdReduceAlu", opLat=3),
        # SVE FP associative add (fadda)
        OpDesc(opClass="SimdFloatReduceAdd", opLat=2, pipelined=True),
        # misc insts (vneg)-3.17
        OpDesc(opClass="SimdFloatMisc", opLat=2),
        # PREDALU has instructions in V group ( trn1 and uzp )
        OpDesc(opClass="SimdPredAlu", opLat=2),
        # V13
        # SVE reduction, arith, S form (smaxv)-3.16
        OpDesc(opClass="SimdReduceCmp", opLat=2),
        # Vec absolute diff accum (vaba) -3.16
        OpDesc(opClass="SimdAddAcc", opLat=4),
        # Vec shift by immed, (shl)-3.16
        OpDesc(opClass="SimdShift", opLat=2),
        # Vec shift accumulate (vsra)-3.16
        OpDesc(opClass="SimdShiftAcc", opLat=4),
    ]
    count = 1


class O3_ARM_Grace_Simple_Int(FUDesc):
    """
    This class refers to pipelines Branch0, Integer single Cycles 0,
    Integer single Cycle 1 (symbol B and S in (2) table 3)
    """

    # Aarch64 ALU (Unfortunately branches are put together with IntALU)
    opList = [OpDesc(opClass="IntAlu", opLat=1)]
    count = 6


class O3_ARM_Grace_Complex_Int(FUDesc):
    """
    This class refers to pipelines integer single/multicycle 1
    (this refers to pipeline symbol M in (2) table 3)
    """

    # Aarch64 Int ALU
    opList = [
        OpDesc(opClass="IntAlu", opLat=1),  #  Int ALU
        OpDesc(opClass="IntMult", opLat=2),  #  Int mult
        # Int divide W-form (sdiv)- we take average
        OpDesc(opClass="IntDiv", opLat=5, pipelined=True),
        OpDesc(opClass="IprAccess", opLat=1),  #  Prefetch
        # Latency varies alot for different instructions in this group.
        OpDesc(opClass="SimdPredAlu"),
    ]
    count = 2


class O3_ARM_Grace_LoadStore(FUDesc):
    """
    This class refers to Load/Store0/1
    (symbol L in Neoverse guide table 3-1)
    """

    opList = [
        OpDesc(opClass="MemRead"),
        OpDesc(opClass="FloatMemRead"),
        OpDesc(opClass="MemWrite"),
        OpDesc(opClass="FloatMemWrite"),
    ]
    count = 2


class O3_ARM_Grace_Load(FUDesc):
    opList = [OpDesc(opClass="MemRead"), OpDesc(opClass="FloatMemRead")]
    count = 1


class O3_ARM_Grace_Store(FUDesc):
    opList = [OpDesc(opClass="MemWrite"), OpDesc(opClass="FloatMemWrite")]
    count = 2


class O3_ARM_Grace_FUP(FUPool):
    FUList = [
        O3_ARM_Grace_FP_Vec_0(),
        O3_ARM_Grace_FP_Vec_1(),
        O3_ARM_Grace_FP_Vec_2(),
        O3_ARM_Grace_FP_Vec_3(),
        O3_ARM_Grace_Simple_Int(),  # 6
        O3_ARM_Grace_Complex_Int(),  # 2
        O3_ARM_Grace_LoadStore(),  #
        O3_ARM_Grace_Load(),
        O3_ARM_Grace_Store(),
    ]


# class O3_ARM_Grace_BP(BiModeBP):
#     """
#     Bi-Mode Branch Predictor
#     """

#     globalPredictorSize = 8192
#     globalCtrBits = 2
#     choicePredictorSize = 8192
#     choiceCtrBits = 2
#     btb = SimpleBTB(numEntries=4096, tagBits=18)
#     RASSize = 16
#     instShiftAmt = 2
class O3_ARM_Grace_BP(BiModeBP):
    globalPredictorSize = 8192
    globalCtrBits = 2
    choicePredictorSize = 8192
    choiceCtrBits = 2
    BTBEntries = 4096
    BTBTagSize = 18
    RASSize = 16
    instShiftAmt = 2



class Grace4Wide(ArmO3CPU):
    #def __init__(self):
    #    super().__init__()

    commitToIEWDelay = 1
    commitToRenameDelay = 1
    iewToRenameDelay = 1
    commitToDecodeDelay = 1
    iewToDecodeDelay = 1
    renameToDecodeDelay = 1
    commitToFetchDelay = 1
    iewToFetchDelay = 1
    renameToFetchDelay = 1
    decodeToFetchDelay = 1
    fetchWidth = 4
    fetchBufferSize = 64
    fetchToDecodeDelay = 1
    decodeWidth = 8
    decodeToRenameDelay = 1
    renameWidth = 8
    renameToIEWDelay = 1
    issueToExecuteDelay = 1
    dispatchWidth = 8
    issueWidth = 4
    wbWidth = 8
    iewToCommitDelay = 1
    renameToROBDelay = 1
    commitWidth = 8
    squashWidth = 8
    trapLatency = 1
    backComSize = 5
    forwardComSize = 5

    numROBEntries = 320
    numPhysFloatRegs = 192
    numPhysVecRegs = 192
    numPhysIntRegs = 224

    numIQEntries = 120

    # cacheStorePorts = 512
    # cacheLoadPorts = 512

    switched_out = False
    branchPred = O3_ARM_Grace_BP()
    fuPool = O3_ARM_Grace_FUP()

    LQEntries = 68
    SQEntries = 72
    LSQDepCheckShift = 0
    LFSTSize = 1024
    SSITSize = 1024


class Grace12Wide(ArmO3CPU):
    # def __init__(self):
    #     super().__init__()

    commitToIEWDelay = 1
    commitToRenameDelay = 1
    iewToRenameDelay = 1
    commitToDecodeDelay = 1
    iewToDecodeDelay = 1
    renameToDecodeDelay = 1
    commitToFetchDelay = 1
    iewToFetchDelay = 1
    renameToFetchDelay = 1
    decodeToFetchDelay = 1
    fetchWidth = 12
    fetchBufferSize = 64
    fetchToDecodeDelay = 1
    decodeWidth = 8
    decodeToRenameDelay = 1
    renameWidth = 8
    renameToIEWDelay = 1
    issueToExecuteDelay = 1
    dispatchWidth = 8
    issueWidth = 12
    wbWidth = 8
    iewToCommitDelay = 1
    renameToROBDelay = 1
    commitWidth = 8
    squashWidth = 8
    trapLatency = 13
    backComSize = 5
    forwardComSize = 5

# 1 thread
    numROBEntries = 320
    numPhysFloatRegs = 192
    numPhysVecRegs = 192 
    numPhysIntRegs = 224
    numIQEntries = 120

# 1.5 resources scaling per thread
# # 2 thread
#     numROBEntries = 480
#     numPhysFloatRegs = 288
#     numPhysVecRegs = 288 
#     numPhysIntRegs = 336
#     numIQEntries = 180

# # 4 thread
#     numROBEntries = 720
#     numPhysFloatRegs = 432
#     numPhysVecRegs = 432 
#     numPhysIntRegs = 504
#     numIQEntries = 270

# # 8 thread
#     numROBEntries = 1080
#     numPhysFloatRegs = 648
#     numPhysVecRegs = 648 
#     numPhysIntRegs = 756
#     numIQEntries = 405

# resource constraints
# # 2 threads
#     numROBEntries = 384
#     numPhysFloatRegs = 230
#     numPhysVecRegs = 230 
#     numPhysIntRegs = 269
#     numIQEntries = 144

# # 4 threads
#     numROBEntries = 448
#     numPhysFloatRegs = 269
#     numPhysVecRegs = 269 
#     numPhysIntRegs = 314
#     numIQEntries = 168

# # 6 threads
#     numROBEntries = 512
#     numPhysFloatRegs = 308
#     numPhysVecRegs = 308 
#     numPhysIntRegs = 359
#     numIQEntries = 192

# # 8 threads
#     numROBEntries = 580 
#     numPhysFloatRegs = 346
#     numPhysVecRegs = 396 
#     numPhysIntRegs = 404
#     numIQEntries = 216

# # 10 threads
#     numROBEntries = 640
#     numPhysFloatRegs = 384
#     numPhysVecRegs = 484 
#     numPhysIntRegs = 448
#     numIQEntries = 240

    # cacheStorePorts = 512
    # cacheLoadPorts = 512

    switched_out = False
    branchPred = O3_ARM_Grace_BP()
    fuPool = O3_ARM_Grace_FUP()

    LQEntries = 68
    SQEntries = 72
    LSQDepCheckShift = 0
    LFSTSize = 1024
    SSITSize = 1024


class Grace12Wide_testConfig(ArmO3CPU):
    # def __init__(self):
    #     super().__init__()

    # 12 wide
    commitToIEWDelay = 1
    commitToRenameDelay = 1
    iewToRenameDelay = 1
    commitToDecodeDelay = 1
    iewToDecodeDelay = 1
    renameToDecodeDelay = 1
    commitToFetchDelay = 1
    iewToFetchDelay = 1
    renameToFetchDelay = 1
    decodeToFetchDelay = 1
    fetchWidth = 12
    fetchBufferSize = 64
    fetchToDecodeDelay = 1
    decodeWidth = 8
    decodeToRenameDelay = 1
    renameWidth = 8
    renameToIEWDelay = 1
    issueToExecuteDelay = 1
    dispatchWidth = 8
    issueWidth = 12
    wbWidth = 8
    iewToCommitDelay = 1
    renameToROBDelay = 1
    commitWidth = 8
    squashWidth = 8
    trapLatency = 1
    backComSize = 5
    forwardComSize = 5

    # 4 wide
    # commitToIEWDelay = 1
    # commitToRenameDelay = 1
    # iewToRenameDelay = 1
    # commitToDecodeDelay = 1
    # iewToDecodeDelay = 1
    # renameToDecodeDelay = 1
    # commitToFetchDelay = 1
    # iewToFetchDelay = 1
    # renameToFetchDelay = 1
    # decodeToFetchDelay = 1
    # fetchWidth = 12
    # fetchBufferSize = 64
    # fetchToDecodeDelay = 1
    # decodeWidth = 8
    # decodeToRenameDelay = 1
    # renameWidth = 4
    # renameToIEWDelay = 1
    # issueToExecuteDelay = 1
    # dispatchWidth = 4
    # issueWidth = 4
    # wbWidth = 4
    # iewToCommitDelay = 1
    # renameToROBDelay = 1
    # commitWidth = 4
    # squashWidth = 4
    # trapLatency = 1
    # backComSize = 5
    # forwardComSize = 5

    # 1 thread values
    # numROBEntries = 512
    # numPhysFloatRegs = 250
    # numPhysVecRegs = 250 
    # numPhysIntRegs = 250
    # numIQEntries = 120

    # switched_out = False
    # branchPred = O3_ARM_Grace_BP()
    # fuPool = O3_ARM_Grace_FUP()

    # LQEntries = 68
    # SQEntries = 72
    # LSQDepCheckShift = 0
    # LFSTSize = 1024
    # SSITSize = 1024

    # 4 thread values
    # numROBEntries = 1280
    # numPhysFloatRegs = 432
    # numPhysVecRegs = 432 
    # numPhysIntRegs = 504
    # numIQEntries = 480

    # switched_out = False
    # branchPred = O3_ARM_Grace_BP()
    # fuPool = O3_ARM_Grace_FUP()

    # LQEntries = 170
    # SQEntries = 180
    # LSQDepCheckShift = 0
    # LFSTSize = 1024
    # SSITSize = 1024

    # Running config
    
    numPhysFloatRegs = 432
    numPhysVecRegs = 432 
    numPhysIntRegs = 504

    numIQEntries = 300
    numSIQEntries = 300
    numROBEntries = 230
    #numWIQEntries = 120
    numWIQEntries = 10
    LQEntries = 340
    SQEntries = 180

    # numIQEntries = 2000
    # numSIQEntries = 2000
    # numROBEntries = 2000
    # numWIQEntries = 120
    # LQEntries = 2000
    # SQEntries = 2000

    switched_out = False
    branchPred = O3_ARM_Grace_BP()
    fuPool = O3_ARM_Grace_FUP()

    LSQDepCheckShift = 0
    LFSTSize = 1024
    SSITSize = 1024

    smtLSQPolicy = "Partitioned"
    smtROBPolicy = "Partitioned"
    smtIQPolicy = "SDynamicWStatic"
    #smtIQPolicy = "SDynamicWStatic"
    # smtFetchPolicy = "SWFetchCount"
    smtFetchPolicy = "IQCount"

class Grace12Wide_1thread(ArmO3CPU):
    # def __init__(self):
    #     super().__init__()

    commitToIEWDelay = 1
    commitToRenameDelay = 1
    iewToRenameDelay = 1
    commitToDecodeDelay = 1
    iewToDecodeDelay = 1
    renameToDecodeDelay = 1
    commitToFetchDelay = 1
    iewToFetchDelay = 1
    renameToFetchDelay = 1
    decodeToFetchDelay = 1
    fetchWidth = 12
    fetchBufferSize = 64
    fetchToDecodeDelay = 1
    decodeWidth = 8
    decodeToRenameDelay = 1
    renameWidth = 8
    renameToIEWDelay = 1
    issueToExecuteDelay = 1
    dispatchWidth = 8
    issueWidth = 12
    wbWidth = 8
    iewToCommitDelay = 1
    renameToROBDelay = 1
    commitWidth = 8
    squashWidth = 8
    trapLatency = 1
    backComSize = 5
    forwardComSize = 5

    numROBEntries = 230
    numPhysFloatRegs = 250
    numPhysVecRegs = 250 
    numPhysIntRegs = 250
    numIQEntries = 120

    switched_out = False
    branchPred = O3_ARM_Grace_BP()
    fuPool = O3_ARM_Grace_FUP()

    LQEntries = 68
    SQEntries = 72
    LSQDepCheckShift = 0
    LFSTSize = 1024
    SSITSize = 1024

    smtLSQPolicy = "Partitioned"
    smtROBPolicy = "Partitioned"
    smtIQPolicy = "Partitioned"
    smtFetchPolicy = "SWFetchCount"
class Grace12Wide_2thread(ArmO3CPU):
    # def __init__(self):
    #     super().__init__()

    commitToIEWDelay = 1
    commitToRenameDelay = 1
    iewToRenameDelay = 1
    commitToDecodeDelay = 1
    iewToDecodeDelay = 1
    renameToDecodeDelay = 1
    commitToFetchDelay = 1
    iewToFetchDelay = 1
    renameToFetchDelay = 1
    decodeToFetchDelay = 1
    fetchWidth = 12
    fetchBufferSize = 64
    fetchToDecodeDelay = 1
    decodeWidth = 8
    decodeToRenameDelay = 1
    renameWidth = 8
    renameToIEWDelay = 1
    issueToExecuteDelay = 1
    dispatchWidth = 8
    issueWidth = 12
    wbWidth = 8
    iewToCommitDelay = 1
    renameToROBDelay = 1
    commitWidth = 8
    squashWidth = 8
    trapLatency = 1
    backComSize = 5
    forwardComSize = 5

    numROBEntries = 640
    numPhysFloatRegs = 288
    numPhysVecRegs = 288 
    numPhysIntRegs = 336
    numIQEntries = 240
    #numIQEntries = 200

    switched_out = False
    branchPred = O3_ARM_Grace_BP()
    fuPool = O3_ARM_Grace_FUP()

    LQEntries = 102
    SQEntries = 110
    # SQEntries = 150
    LSQDepCheckShift = 0
    LFSTSize = 1024
    SSITSize = 1024

    smtLSQPolicy = "Partitioned"
    smtROBPolicy = "Partitioned"
    smtIQPolicy = "Partitioned"
    smtFetchPolicy = "SWFetchCount"

class Grace12Wide_4thread(ArmO3CPU):
    # def __init__(self):
    #     super().__init__()

    commitToIEWDelay = 1
    commitToRenameDelay = 1
    iewToRenameDelay = 1
    commitToDecodeDelay = 1
    iewToDecodeDelay = 1
    renameToDecodeDelay = 1
    commitToFetchDelay = 1
    iewToFetchDelay = 1
    renameToFetchDelay = 1
    decodeToFetchDelay = 1
    fetchWidth = 12
    fetchBufferSize = 64
    fetchToDecodeDelay = 1
    decodeWidth = 8
    decodeToRenameDelay = 1
    renameWidth = 8
    renameToIEWDelay = 1
    issueToExecuteDelay = 1
    dispatchWidth = 8
    issueWidth = 12
    wbWidth = 8
    iewToCommitDelay = 1
    renameToROBDelay = 1
    commitWidth = 8
    squashWidth = 8
    trapLatency = 1
    backComSize = 5
    forwardComSize = 5

    numROBEntries = 1280
    numPhysFloatRegs = 432
    numPhysVecRegs = 432 
    numPhysIntRegs = 504
    numIQEntries = 480

    switched_out = False
    branchPred = O3_ARM_Grace_BP()
    fuPool = O3_ARM_Grace_FUP()

    LQEntries = 170
    SQEntries = 180
    LSQDepCheckShift = 0
    LFSTSize = 1024
    SSITSize = 1024

    smtLSQPolicy = "Partitioned"
    smtROBPolicy = "Partitioned"
    smtIQPolicy = "Partitioned"
    smtFetchPolicy = "SWFetchCount"

class Grace12Wide_8thread(ArmO3CPU):
    # def __init__(self):
    #     super().__init__()

    commitToIEWDelay = 1
    commitToRenameDelay = 1
    iewToRenameDelay = 1
    commitToDecodeDelay = 1
    iewToDecodeDelay = 1
    renameToDecodeDelay = 1
    commitToFetchDelay = 1
    iewToFetchDelay = 1
    renameToFetchDelay = 1
    decodeToFetchDelay = 1
    fetchWidth = 12
    fetchBufferSize = 64
    fetchToDecodeDelay = 1
    decodeWidth = 8
    decodeToRenameDelay = 1
    renameWidth = 8
    renameToIEWDelay = 1
    issueToExecuteDelay = 1
    dispatchWidth = 8
    issueWidth = 12
    wbWidth = 8
    iewToCommitDelay = 1
    renameToROBDelay = 1
    commitWidth = 8
    squashWidth = 8
    trapLatency = 1
    backComSize = 5
    forwardComSize = 5

    numROBEntries = 2560
    # numROBEntries = 320
    numPhysFloatRegs = 648
    numPhysVecRegs = 648 
    numPhysIntRegs = 756
    numIQEntries = 960

    switched_out = False
    branchPred = O3_ARM_Grace_BP()
    fuPool = O3_ARM_Grace_FUP()

    LQEntries = 306
    SQEntries = 324
    #LQEntries = 544
    #SQEntries = 544
    LSQDepCheckShift = 0
    LFSTSize = 1024
    SSITSize = 1024

    smtLSQPolicy = "Partitioned"
    smtROBPolicy = "Partitioned"
    smtIQPolicy = "Partitioned"
    smtFetchPolicy = "SWFetchCount"

# Instruction Cache
class O3_ARM_grace_ICache(Cache):
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 12
    tgts_per_mshr = 8
    size = "32kB"
    assoc = 2
    is_read_only = True
    # Writeback clean lines as well
    writeback_clean = True

class O3_ARM_grace_ICache_Strong(Cache):
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 12
    tgts_per_mshr = 8
    size = "32kB"
    assoc = 2
    # write_buffers = 16
    # Consider the L2 a victim cache also for clean lines
    writeback_clean = True

class O3_ARM_grace_ICache_Weak(Cache):
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 6
    tgts_per_mshr = 8
    size = "32kB"
    assoc = 2
    write_buffers = 16
    # Consider the L2 a victim cache also for clean lines
    writeback_clean = True
    
# Data Cache
class O3_ARM_grace_DCache(Cache):
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 6
    tgts_per_mshr = 8
    #size = "32kB"
    size = "64kB"
    assoc = 2
    write_buffers = 16
    # Consider the L2 a victim cache also for clean lines
    writeback_clean = True


# L2 Cache
class O3_ARM_grace_L2(Cache):
    # tag_latency = 12
    # data_latency = 12
    # response_latency = 12
    tag_latency = 12
    data_latency = 12
    response_latency = 12
    mshrs = 16
    tgts_per_mshr = 8
    size = "1MB"
    assoc = 16
    write_buffers = 8
    prefetch_on_access = True
    clusivity = "mostly_excl"
    # Simple stride prefetcher
    prefetcher = StridePrefetcher(degree=8, latency=1)
    tags = BaseSetAssoc()
    replacement_policy = RandomRP()