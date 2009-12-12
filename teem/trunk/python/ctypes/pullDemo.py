##
##  teem.py: automatically-generated ctypes python wrappers for Teem
##  Copyright (C) 2009, Gordon Kindlmann
##
##  Permission is hereby granted, free of charge, to any person obtaining
##  a copy of this software and associated documentation files (the
##  "Software"), to deal in the Software without restriction, including
##  without limitation the rights to use, copy, modify, merge, publish,
##  distribute, sublicense, and/or sell copies of the Software, and to
##  permit persons to whom the Software is furnished to do so, subject to
##  the following conditions:
##
##  The above copyright notice and this permission notice shall be
##  included in all copies or substantial portions of the Software.
##
##  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
##  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
##  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
##  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
##  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
##  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
##  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
##

'''
This is really hastily written, and doesn't conform in the least
to good conventions of Python coding. None of the "API" here, such as
it is, should be expected to survive in a future Teem version. The
code here is mainly to serve as a demo of using the pull library in Teem
(whose API should be largely stable), and to simplify documenting
using pull to produce typical results.
'''

import teem
import ctypes
import sys

##
## volLoad: loads in volumes
##
def volLoad(vlist, cachePath, kssBlurStr, verbose):
    kSSblur = teem.nrrdKernelSpecNew()
    if (teem.nrrdKernelSpecParse(kSSblur, kssBlurStr)):
        estr = teem.biffGetDone('nrrd')
        print "problem with kernel:\n%s" % estr
        sys.exit(1)
    vol = (ctypes.POINTER(teem.meetPullVol) * len(vlist))()
    E = 0
    for i in range(len(vlist)):
        vol[i] = teem.meetPullVolNew()
        if (not E):
            E += teem.meetPullVolParse(vol[i], vlist[i])
    if (not E): 
        E += teem.meetPullVolLoadMulti(vol, len(vol),
                                       cachePath, kSSblur, verbose)
    if (E):
        estr = teem.biffGetDone('meet')
        print "problem with volumes:\n%s" % estr
        sys.exit(1)
    teem.nrrdKernelSpecNix(kSSblur)
    return vol

def volFree(vol):
    for i in range(len(vol)):
        vol[i] = teem.meetPullVolNuke(vol[i])

##
## especParse: returns a length-4 list of the arguments to pass 
## to pullInterEnergySet. Arguments are:
## type: from pullIterType* enum
## r: string definition for energy function along space (especR)
## s: string definition for energy function along scale (especS)
## w: string definition for windowing energy function (especW)
##
def energyParse(**kwargs):
    type = kwargs['type']
    if (teem.pullInterTypeJustR == type
        or teem.pullInterTypeUnivariate == type):
        espec = [teem.pullEnergySpecNew(), None, None]
    elif (teem.pullInterTypeSeparable == type):
        espec = [teem.pullEnergySpecNew(),
                 teem.pullEnergySpecNew(), None]
    elif (teem.pullInterTypeAdditive == type):
        espec = [teem.pullEnergySpecNew(),
                 teem.pullEnergySpecNew(),
                 teem.pullEnergySpecNew()]
    lett = ['r', 's', 'w']
    E = 0
    for i in range(3):
        if (not E and espec[i]):
            E += teem.pullEnergySpecParse(espec[i], kwargs[lett[i]])
    if (E):
        estr = teem.biffGetDone('pull')
        print "problem with infos:\n%s" % estr
        sys.exit(1)
    ret = [type, espec[0], espec[1], espec[2]]
    return ret

def energyFree(espec):
    espec[1] = teem.pullEnergySpecNix(espec[1])
    espec[2] = teem.pullEnergySpecNix(espec[2])
    espec[3] = teem.pullEnergySpecNix(espec[3])
    return

##
## initSet: calls (and returns value from) the appropriate 
## pullInitMethod* function. Possibilities are:
## [teem.pullInitMethodRandom, num]
## [teem.pullInitMethodPointPerVoxel, 
##     ppv, zslcmin, zslcmax, alongscalenum, jitter]
## [teem.pullInitMethodGivenPos, npos]
##
def initSet(pctx, info):
    if (teem.pullInitMethodRandom == info[0]):
        return teem.pullInitRandomSet(pctx, info[1])
    elif (teem.pullInitMethodPointPerVoxel == info[0]):
        return teem.pullInitPointPerVoxelSet(pctx, info[1],
                                             info[2], info[3],
                                             info[4], info[5])
    elif (teem.pullInitMethodGivenPos == info[0]):
        return teem.pullInitGivenPosSet(pctx, info[1])


## place for storing pullContext->sysParm.gamma
gamma = 0

##
## All the set-up and commands that are required to do a pullContext run
##
def run(nposOut, **args):
    global gamma
    if (not ('vol' in args and 'info' in args and 'efs' in args)):
        print "run: didn't get vol, info, and efs args"
        sys.exit(1)
        
    # learn args, with defaults
    vol = args.get('vol')
    infoStr = args.get('info')
    efs = args.get('efs')
    init = args.get('init', [teem.pullInitMethodRandom, 100])
    energyDict = args.get('energy', {'type':teem.pullInterTypeJustR, 
                                     'r':'cwell:0.6,-0.002'})
    verbose = args.get('verbose', 1)
    rngSeed = args.get('rngSeed', 42)
    nave = args.get('nave', True)
    cbstr = args.get('cbstr', True)
    ratb = args.get('ratb', True)
    lti = args.get('lti', False)
    npcwza = args.get('npcwza', False)
    iterMax = args.get('iterMax', 100)
    snap = args.get('snap', 0)
    pcp = args.get('pcp', 5)
    radSpace = args.get('radSpace', 1)
    radScale = args.get('radScale', 1)
    alpha = args.get('alpha', 0.5)
    beta = args.get('beta', 0.5)
    gamma = args.get('gamma', 1)
    stepInitial = args.get('step', 1)
    ssBack = args.get('ssBack', 0.2)
    ssOppor = args.get('ssOppor', 1.1)
    pbm = args.get('pbm', 50)
    k00Str = args.get('k00', 'c4h')
    k11Str = args.get('k11', 'c4hd')
    k22Str = args.get('k22', 'c4hdd')
    kSSreconStr = args.get('kSSrecon', 'hermite')
    eip = args.get('eip', 0.00005)
    edmin = args.get('edmin', 0.00001)
    edpcmin = args.get('edpcmin', 0.01)

    # create pullContext and set up all its state.  Its kind of silly that
    # the pullContext is created anew everytime we want to run it, but thats
    # because currently the pullContext doesn't have the kind of internal
    # network of flags (like the gageContext does) that allow the context
    # to be modifyied and re-used indefinitely.  This will be fixed after
    # the Teem v1.11 release.
    pctx = teem.pullContextNew()
    energyList = energyParse(**energyDict)
    if (teem.pullVerboseSet(pctx, verbose) or
        teem.pullRngSeedSet(pctx, rngSeed) or
        teem.pullFlagSet(pctx, teem.pullFlagNixAtVolumeEdgeSpace, nave) or
        teem.pullFlagSet(pctx, teem.pullFlagConstraintBeforeSeedThresh, 
                         cbstr) or
        teem.pullFlagSet(pctx, teem.pullFlagEnergyFromStrength, efs) or
        teem.pullFlagSet(pctx, teem.pullFlagRestrictiveAddToBins, ratb) or
        teem.pullFlagSet(pctx, teem.pullFlagNoPopCntlWithZeroAlpha, npcwza) or
        teem.pullIterParmSet(pctx, teem.pullIterParmMax, iterMax) or
        teem.pullIterParmSet(pctx, teem.pullIterParmSnap, snap) or
        teem.pullIterParmSet(pctx, teem.pullIterParmPopCntlPeriod, pcp) or
        teem.pullSysParmSet(pctx, teem.pullSysParmStepInitial, stepInitial) or
        teem.pullSysParmSet(pctx, teem.pullSysParmRadiusSpace, radSpace) or
        teem.pullSysParmSet(pctx, teem.pullSysParmRadiusScale, radScale) or
        teem.pullSysParmSet(pctx, teem.pullSysParmAlpha, alpha) or
        teem.pullSysParmSet(pctx, teem.pullSysParmBeta, beta) or
        teem.pullSysParmSet(pctx, teem.pullSysParmGamma, gamma) or
        teem.pullSysParmSet(pctx, teem.pullSysParmEnergyIncreasePermit, eip) or
        teem.pullSysParmSet(pctx, teem.pullSysParmEnergyDecreaseMin, edmin) or
        teem.pullSysParmSet(pctx, teem.pullSysParmEnergyDecreasePopCntlMin,
                            edpcmin) or
        teem.pullSysParmSet(pctx, teem.pullSysParmBackStepScale, ssBack) or
        teem.pullSysParmSet(pctx, teem.pullSysParmOpporStepScale, ssOppor) or
        teem.pullProgressBinModSet(pctx, pbm) or
        teem.pullInterEnergySet(pctx, energyList[0], energyList[1],
                                energyList[2], energyList[3])):
        estr = teem.biffGetDone('pull')
        print "problem with set-up:\n%s" % estr
        sys.exit(1)

    # The meet library offers a slightly higher-level abstraction of
    # the pullInfo; this code parses a bunch of info specifications
    info = (ctypes.POINTER(teem.meetPullInfo) * len(infoStr))()
    for i in range(len(infoStr)):
        info[i] = teem.meetPullInfoNew()
        if (teem.meetPullInfoParse(info[i], infoStr[i])):
            estr = teem.biffGetDone('meet')
            print "problem with infos:\n%s" % estr
            sys.exit(1)

    # Create all the kernels needed for reconstruction
    k00 = teem.nrrdKernelSpecNew()
    k11 = teem.nrrdKernelSpecNew()
    k22 = teem.nrrdKernelSpecNew()
    kSSrecon = teem.nrrdKernelSpecNew()
    if (teem.nrrdKernelSpecParse(k00, k00Str) or
        teem.nrrdKernelSpecParse(k11, k11Str) or
        teem.nrrdKernelSpecParse(k22, k22Str) or
        teem.nrrdKernelSpecParse(kSSrecon, kSSreconStr)):
        estr = teem.biffGetDone('nrrd')
        print "problem with kernels:\n%s" % estr
        sys.exit(1)

    # We do assume that the volumes came in loaded (as opposed to reloading
    # them here for every one); here we add all the volumes and infos into
    # the pullContext
    if (teem.meetPullVolAddMulti(pctx, vol, len(vol),
                                 k00, k11, k22, kSSrecon) or
        teem.meetPullInfoAddMulti(pctx, info, len(info))):
            estr = teem.biffGetDone('meet')
            print "problem with infos:\n%s" % estr
            sys.exit(1)

    # Final setup and the actual "pullRun" call
    if (initSet(pctx, init) or
        teem.pullInitLiveThreshUseSet(pctx, lti) or
        teem.pullStart(pctx) or
        teem.pullRun(pctx) or
        teem.pullOutputGet(nposOut, None, None, None, 0, pctx)):
        estr = teem.biffGetDone('pull')
        print "problem running system:\n%s" % estr
        sys.exit(1)

    # until there's a clean way to re-use a pullContext for different runs,
    # there's no good way for the user to call pullGammaLearn (because pctx
    # doesn't survive outside the call to "run".  So we call pullGammaLearn
    # it whenever it makes sense to do so and save the result in the global
    # "gamma"
    if (teem.pullInterTypeAdditive == pctx.contents.interType and
        teem.pullEnergyButterworthParabola.contents.name
        == pctx.contents.energySpecS.contents.energy.contents.name):
        if (teem.pullGammaLearn(pctx)):
            estr = teem.biffGetDone('pull')
            print "problem learning gamma:\n%s" % estr
            sys.exit(1)
        gamma = pctx.contents.sysParm.gamma

    teem.nrrdKernelSpecNix(k00)
    teem.nrrdKernelSpecNix(k11)
    teem.nrrdKernelSpecNix(k22)
    teem.nrrdKernelSpecNix(kSSrecon)
    energyFree(energyList)
    teem.pullContextNix(pctx)
    return
