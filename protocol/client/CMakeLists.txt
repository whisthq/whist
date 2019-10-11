cmake_minimum_required(VERSION 2.8)

project(fprotocol_client)

# for Live555 #
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-fpermissive -std=c++0x") # add fpermissive to silence int* to unsigned int* errors
# endof for Live555

SET(PROJECT_VER_MAJOR 1)
SET(PROJECT_VER_MINOR 0)
SET(PROJECT_VER_PATCH 0)
SET(PROJECT_VER "${PROJECT_VER_MAJOR}.${PROJECT_VER_MINOR}.${PROJECT_VER_PATCH}")
SET(PROJECT_APIVER "${PROJECT_VER}")

#if(ARM64) -- let's not build mmal, we don't need it
set(BUILD_MMAL FALSE)
set(BUILD_MMAL_APPS FALSE)
#else()
#   set(BUILD_MMAL TRUE)
#   set(BUILD_MMAL_APPS TRUE)
#endif()
set(vmcs_root ${PROJECT_SOURCE_DIR})
get_filename_component(VIDEOCORE_ROOT . ABSOLUTE)

set(VCOS_PTHREADS_BUILD_SHARED TRUE)

include(makefiles/cmake/global_settings.cmake)
include(makefiles/cmake/arm-linux.cmake)
include(makefiles/cmake/vmcs.cmake)

enable_language(ASM)

# Global include paths
include_directories(application/framework)
include_directories(${PROJECT_SOURCE_DIR})
include_directories(interface/vcos/pthreads)
include_directories(interface/vmcs_host/linux)
include_directories(interface/vmcs_host)
include_directories(interface/vmcs_host/khronos)
include_directories(interface/khronos/include)
include_directories(${PROJECT_BINARY_DIR})
include_directories(interface/vchiq_arm)
#include_directories(tools/inet_transport)
include_directories(host_support/include)

# Global compiler flags
if(CMAKE_COMPILER_IS_GNUCC)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-multichar -Wall -Wno-unused-but-set-variable -fPIC")
endif()

add_definitions(-D_REENTRANT)
add_definitions(-DUSE_VCHIQ_ARM -DVCHI_BULK_ALIGN=1 -DVCHI_BULK_GRANULARITY=1)
add_definitions(-DOMX_SKIP64BIT)
add_definitions(-DEGL_SERVER_DISPMANX)
add_definitions(-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
add_definitions(-D_GNU_SOURCE)

# do we actually need this?
add_definitions(-D__VIDEOCORE4__)
add_definitions(-DTV_SUPPORTED_MODE_NO_DEPRECATED)

# add_definitions(-DKHRONOS_CLIENT_LOGGING)

# Check for OpenWF-C value set via command line
if(KHRONOS_EGL_PLATFORM MATCHES "openwfc")
   add_definitions(-DKHRONOS_EGL_PLATFORM_OPENWFC)
endif()

# List of subsidiary CMakeLists
add_subdirectory(interface/vcos)
add_subdirectory(interface/vmcs_host)
add_subdirectory(interface/vchiq_arm)
if(NOT ARM64)
   add_subdirectory(interface/khronos)
endif()

#add_subdirectory(opensrc/tools/lua)
if(BUILD_MMAL)
   include_directories(interface/mmal)
   add_subdirectory(interface/mmal)
   add_subdirectory(containers)
endif()

# VidTex supports Android and Linux -- don't build mmal apps on RPi, not using Android
#if(BUILD_MMAL_APPS)
#add_subdirectory(application/android/apps/vidtex)
#endif(BUILD_MMAL_APPS)

if(NOT ARM64)
   add_subdirectory(middleware/openmaxil)
endif()

# 3d demo code
#if(NOT ANDROID)
#   add_subdirectory(thirdparty/applications/demos)
#   add_subdirectory(opensrc/applications/demos)
#endif()

#if(ENABLE_3D_TESTS)
#   add_subdirectory(thirdparty/applications/test)
#endif()

# FIXME: we should use a pre-packaged version of freetype
# rather than the one included in the repo.
#add_subdirectory(opensrc/helpers/freetype)
#add_subdirectory(${PROJECT_SOURCE_DIR}/opensrc/helpers/fonts/ttf-bitstream-vera)

# VMCS Host Applications
#add_subdirectory(application/framework)

# add_subdirectory(interface/vchiq/test/win32)

# Apps and libraries supporting Camera Tuning Tool
#add_subdirectory(tools/inet_transport/linux)
#add_subdirectory(host_support/vcstandalone)

# add linux apps
add_subdirectory(application/protocol)
add_subdirectory(opensrc/helpers/libfdt)
add_subdirectory(helpers/dtoverlay)

set(fractal_protocol_VERSION_MAJOR 1)
set(fractal_protocol_VERSION_MINOR 0)

include_directories("${PROJECT_BINARY_DIR}")
include(FindPkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	# Produce a pkg-config file
	foreach(PCFILE bcm_host.pc egl.pc glesv2.pc vg.pc brcmegl.pc brcmglesv2.pc  brcmvg.pc vcsm.pc mmal.pc )
		configure_file("pkgconfig/${PCFILE}.in" "${PCFILE}" @ONLY)
		install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/${PCFILE}"
			DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
	endforeach()
endif()
# Remove cache entry, if one added by command line
unset(KHRONOS_EGL_PLATFORM CACHE)

####### Now that we got everything for the standard Raspberry Pi, let's deal with the video.c and video.h modified files

# add folders for HELLO PI LIBS for linking to video.c
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/application/protocol/libs/bcm_host)
include_directories(${PROJECT_SOURCE_DIR}/application/protocol/libs/bcm_host/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/application/protocol/protocol-app/pi/libs/ilclient)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/application/protocol/protocol-app/pi/libs/vgfont)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/application/protocol/protocol-app/pi/libs/revision)

set(ILCLIENT_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/application/protocol/protocol-app/pi/libs/ilclient/ilclient.c ${CMAKE_CURRENT_SOURCE_DIR}/application/protocol/protocol-app/pi/libs/ilclient/ilcore.c)
add_library(ilclient ${ILCLIENT_SRCS})

set(REVISION_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/application/protocol/protocol-app/pi/libs/revision/revision.c )
add_library(revision ${REVISION_SRCS})

set(HELLO_PI_LIBS ilclient openmaxil bcm_host vcos vchiq_arm revision)

####### Now that we got everything for the Raspberry Pi, let's get everything for Live555

# include main paths directories
include_directories(BasicUsageEnvironment)
include_directories(BasicUsageEnvironment/include)
include_directories(groupsock)
include_directories(groupsock/include)
include_directories(liveMedia)
include_directories(liveMedia/include)
include_directories(UsageEnvironment)
include_directories(UsageEnvironment/include)

# executable protocol with everything (live and video.c)
add_executable(fprotocol_client
	application/protocol/protocol-app/pi/video_decode_play/video.h
	application/protocol/protocol-app/pi/video_decode_play/video.c # now we will compile them at the same time
	BasicUsageEnvironment/include/BasicHashTable.hh
        BasicUsageEnvironment/include/BasicUsageEnvironment.hh
        BasicUsageEnvironment/include/BasicUsageEnvironment0.hh
        BasicUsageEnvironment/include/BasicUsageEnvironment_version.hh
        BasicUsageEnvironment/include/DelayQueue.hh
        BasicUsageEnvironment/include/HandlerSet.hh
        BasicUsageEnvironment/BasicHashTable.cpp
        BasicUsageEnvironment/BasicTaskScheduler.cpp
        BasicUsageEnvironment/BasicTaskScheduler0.cpp
        BasicUsageEnvironment/BasicUsageEnvironment.cpp
        BasicUsageEnvironment/BasicUsageEnvironment0.cpp
        BasicUsageEnvironment/DelayQueue.cpp
        groupsock/include/GroupEId.hh
        groupsock/include/Groupsock.hh
        groupsock/include/groupsock_version.hh
        groupsock/include/GroupsockHelper.hh
        groupsock/include/IOHandlers.hh
        groupsock/include/NetAddress.hh
        groupsock/include/NetCommon.h
        groupsock/include/NetInterface.hh
        groupsock/include/TunnelEncaps.hh
        groupsock/GroupEId.cpp
        groupsock/Groupsock.cpp
        groupsock/GroupsockHelper.cpp
        groupsock/inet.c
        groupsock/IOHandlers.cpp
        groupsock/NetAddress.cpp
        groupsock/NetInterface.cpp
        liveMedia/include/AC3AudioFileServerMediaSubsession.hh
        liveMedia/include/AC3AudioRTPSink.hh
        liveMedia/include/AC3AudioRTPSource.hh
        liveMedia/include/AC3AudioStreamFramer.hh
        liveMedia/include/ADTSAudioFileServerMediaSubsession.hh
        liveMedia/include/ADTSAudioFileSource.hh
        liveMedia/include/AMRAudioFileServerMediaSubsession.hh
        liveMedia/include/AMRAudioFileSink.hh
        liveMedia/include/AMRAudioFileSource.hh
        liveMedia/include/AMRAudioRTPSink.hh
        liveMedia/include/AMRAudioRTPSource.hh
        liveMedia/include/AMRAudioSource.hh
        liveMedia/include/AudioInputDevice.hh
        liveMedia/include/AudioRTPSink.hh
        liveMedia/include/AVIFileSink.hh
        liveMedia/include/Base64.hh
        liveMedia/include/BasicUDPSink.hh
        liveMedia/include/BasicUDPSource.hh
        liveMedia/include/BitVector.hh
        liveMedia/include/ByteStreamFileSource.hh
        liveMedia/include/ByteStreamMemoryBufferSource.hh
        liveMedia/include/ByteStreamMultiFileSource.hh
        liveMedia/include/DeviceSource.hh
        liveMedia/include/DigestAuthentication.hh
        liveMedia/include/DVVideoFileServerMediaSubsession.hh
        liveMedia/include/DVVideoRTPSink.hh
        liveMedia/include/DVVideoRTPSource.hh
        liveMedia/include/DVVideoStreamFramer.hh
        liveMedia/include/FileServerMediaSubsession.hh
        liveMedia/include/FileSink.hh
        liveMedia/include/FramedFileSource.hh
        liveMedia/include/FramedFilter.hh
        liveMedia/include/FramedSource.hh
        liveMedia/include/GenericMediaServer.hh
        liveMedia/include/GSMAudioRTPSink.hh
        liveMedia/include/H261VideoRTPSource.hh
        liveMedia/include/H263plusVideoFileServerMediaSubsession.hh
        liveMedia/include/H263plusVideoRTPSink.hh
        liveMedia/include/H263plusVideoRTPSource.hh
        liveMedia/include/H263plusVideoStreamFramer.hh
        liveMedia/include/H264or5VideoFileSink.hh
        liveMedia/include/H264or5VideoRTPSink.hh
        liveMedia/include/H264or5VideoStreamDiscreteFramer.hh
        liveMedia/include/H264or5VideoStreamFramer.hh
        liveMedia/include/H264VideoFileServerMediaSubsession.hh
        liveMedia/include/H264VideoFileSink.hh
        liveMedia/include/H264VideoRTPSink.hh
        liveMedia/include/H264VideoRTPSource.hh
        liveMedia/include/H264VideoStreamDiscreteFramer.hh
        liveMedia/include/H264VideoStreamFramer.hh
        liveMedia/include/H265VideoFileServerMediaSubsession.hh
        liveMedia/include/H265VideoFileSink.hh
        liveMedia/include/H265VideoRTPSink.hh
        liveMedia/include/H265VideoRTPSource.hh
        liveMedia/include/H265VideoStreamDiscreteFramer.hh
        liveMedia/include/H265VideoStreamFramer.hh
        liveMedia/include/InputFile.hh
        liveMedia/include/JPEG2000VideoRTPSink.hh
        liveMedia/include/JPEG2000VideoRTPSource.hh
        liveMedia/include/JPEGVideoRTPSink.hh
        liveMedia/include/JPEGVideoRTPSource.hh
        liveMedia/include/JPEGVideoSource.hh
        liveMedia/include/liveMedia.hh
        liveMedia/include/liveMedia_version.hh
        liveMedia/include/Locale.hh
        liveMedia/include/MatroskaFile.hh
        liveMedia/include/MatroskaFileServerDemux.hh
        liveMedia/include/Media.hh
        liveMedia/include/MediaSession.hh
        liveMedia/include/MediaSink.hh
        liveMedia/include/MediaSource.hh
        liveMedia/include/MediaTranscodingTable.hh
        liveMedia/include/MP3ADU.hh
        liveMedia/include/MP3ADUinterleaving.hh
        liveMedia/include/MP3ADURTPSink.hh
        liveMedia/include/MP3ADURTPSource.hh
        liveMedia/include/MP3ADUTranscoder.hh
        liveMedia/include/MP3AudioFileServerMediaSubsession.hh
        liveMedia/include/MP3FileSource.hh
        liveMedia/include/MP3Transcoder.hh
        liveMedia/include/MPEG1or2AudioRTPSink.hh
        liveMedia/include/MPEG1or2AudioRTPSource.hh
        liveMedia/include/MPEG1or2AudioStreamFramer.hh
        liveMedia/include/MPEG1or2Demux.hh
        liveMedia/include/MPEG1or2DemuxedElementaryStream.hh
        liveMedia/include/MPEG1or2DemuxedServerMediaSubsession.hh
        liveMedia/include/MPEG1or2FileServerDemux.hh
        liveMedia/include/MPEG1or2VideoFileServerMediaSubsession.hh
        liveMedia/include/MPEG1or2VideoRTPSink.hh
        liveMedia/include/MPEG1or2VideoRTPSource.hh
        liveMedia/include/MPEG1or2VideoStreamDiscreteFramer.hh
        liveMedia/include/MPEG1or2VideoStreamFramer.hh
        liveMedia/include/MPEG2IndexFromTransportStream.hh
        liveMedia/include/MPEG2TransportFileServerMediaSubsession.hh
        liveMedia/include/MPEG2TransportStreamAccumulator.hh
        liveMedia/include/MPEG2TransportStreamDemux.hh
        liveMedia/include/MPEG2TransportStreamFramer.hh
        liveMedia/include/MPEG2TransportStreamFromESSource.hh
        liveMedia/include/MPEG2TransportStreamFromPESSource.hh
        liveMedia/include/MPEG2TransportStreamIndexFile.hh
        liveMedia/include/MPEG2TransportStreamMultiplexor.hh
        liveMedia/include/MPEG2TransportStreamTrickModeFilter.hh
        liveMedia/include/MPEG2TransportUDPServerMediaSubsession.hh
        liveMedia/include/MPEG4ESVideoRTPSink.hh
        liveMedia/include/MPEG4ESVideoRTPSource.hh
        liveMedia/include/MPEG4GenericRTPSink.hh
        liveMedia/include/MPEG4GenericRTPSource.hh
        liveMedia/include/MPEG4LATMAudioRTPSink.hh
        liveMedia/include/MPEG4LATMAudioRTPSource.hh
        liveMedia/include/MPEG4VideoFileServerMediaSubsession.hh
        liveMedia/include/MPEG4VideoStreamDiscreteFramer.hh
        liveMedia/include/MPEG4VideoStreamFramer.hh
        liveMedia/include/MPEGVideoStreamFramer.hh
        liveMedia/include/MultiFramedRTPSink.hh
        liveMedia/include/MultiFramedRTPSource.hh
        liveMedia/include/OggFile.hh
        liveMedia/include/OggFileServerDemux.hh
        liveMedia/include/OggFileSink.hh
        liveMedia/include/OnDemandServerMediaSubsession.hh
        liveMedia/include/ourMD5.hh
        liveMedia/include/OutputFile.hh
        liveMedia/include/PassiveServerMediaSubsession.hh
        liveMedia/include/ProxyServerMediaSession.hh
        liveMedia/include/QCELPAudioRTPSource.hh
        liveMedia/include/QuickTimeFileSink.hh
        liveMedia/include/QuickTimeGenericRTPSource.hh
        liveMedia/include/RawVideoRTPSink.hh
        liveMedia/include/RawVideoRTPSource.hh
        liveMedia/include/RTCP.hh
        liveMedia/include/RTPInterface.hh
        liveMedia/include/RTPSink.hh
        liveMedia/include/RTPSource.hh
        liveMedia/include/RTSPClient.hh
        liveMedia/include/RTSPCommon.hh
        liveMedia/include/RTSPRegisterSender.hh
        liveMedia/include/RTSPServer.hh
        liveMedia/include/RTSPServerSupportingHTTPStreaming.hh
        liveMedia/include/ServerMediaSession.hh
        liveMedia/include/SimpleRTPSink.hh
        liveMedia/include/SimpleRTPSource.hh
        liveMedia/include/SIPClient.hh
        liveMedia/include/StreamReplicator.hh
        liveMedia/include/T140TextRTPSink.hh
        liveMedia/include/TCPStreamSink.hh
        liveMedia/include/TextRTPSink.hh
        liveMedia/include/TheoraVideoRTPSink.hh
        liveMedia/include/TheoraVideoRTPSource.hh
        liveMedia/include/uLawAudioFilter.hh
        liveMedia/include/VideoRTPSink.hh
        liveMedia/include/VorbisAudioRTPSink.hh
        liveMedia/include/VorbisAudioRTPSource.hh
        liveMedia/include/VP8VideoRTPSink.hh
        liveMedia/include/VP8VideoRTPSource.hh
        liveMedia/include/VP9VideoRTPSink.hh
        liveMedia/include/VP9VideoRTPSource.hh
        liveMedia/include/WAVAudioFileServerMediaSubsession.hh
        liveMedia/include/WAVAudioFileSource.hh
        liveMedia/AC3AudioFileServerMediaSubsession.cpp
        liveMedia/AC3AudioRTPSink.cpp
        liveMedia/AC3AudioRTPSource.cpp
        liveMedia/AC3AudioStreamFramer.cpp
        liveMedia/ADTSAudioFileServerMediaSubsession.cpp
        liveMedia/ADTSAudioFileSource.cpp
        liveMedia/AMRAudioFileServerMediaSubsession.cpp
        liveMedia/AMRAudioFileSink.cpp
        liveMedia/AMRAudioFileSource.cpp
        liveMedia/AMRAudioRTPSink.cpp
        liveMedia/AMRAudioRTPSource.cpp
        liveMedia/AMRAudioSource.cpp
        liveMedia/AudioInputDevice.cpp
        liveMedia/AudioRTPSink.cpp
        liveMedia/AVIFileSink.cpp
        liveMedia/Base64.cpp
        liveMedia/BasicUDPSink.cpp
        liveMedia/BasicUDPSource.cpp
        liveMedia/BitVector.cpp
        liveMedia/ByteStreamFileSource.cpp
        liveMedia/ByteStreamMemoryBufferSource.cpp
        liveMedia/ByteStreamMultiFileSource.cpp
        liveMedia/DeviceSource.cpp
        liveMedia/DigestAuthentication.cpp
        liveMedia/DVVideoFileServerMediaSubsession.cpp
        liveMedia/DVVideoRTPSink.cpp
        liveMedia/DVVideoRTPSource.cpp
        liveMedia/DVVideoStreamFramer.cpp
        liveMedia/EBMLNumber.cpp
        liveMedia/EBMLNumber.hh
        liveMedia/FileServerMediaSubsession.cpp
        liveMedia/FileSink.cpp
        liveMedia/FramedFileSource.cpp
        liveMedia/FramedFilter.cpp
        liveMedia/FramedSource.cpp
        liveMedia/GenericMediaServer.cpp
        liveMedia/GSMAudioRTPSink.cpp
        liveMedia/H261VideoRTPSource.cpp
        liveMedia/H263plusVideoFileServerMediaSubsession.cpp
        liveMedia/H263plusVideoRTPSink.cpp
        liveMedia/H263plusVideoRTPSource.cpp
        liveMedia/H263plusVideoStreamFramer.cpp
        liveMedia/H263plusVideoStreamParser.cpp
        liveMedia/H263plusVideoStreamParser.hh
        liveMedia/H264or5VideoFileSink.cpp
        liveMedia/H264or5VideoRTPSink.cpp
        liveMedia/H264or5VideoStreamDiscreteFramer.cpp
        liveMedia/H264or5VideoStreamFramer.cpp
        liveMedia/H264VideoFileServerMediaSubsession.cpp
        liveMedia/H264VideoFileSink.cpp
        liveMedia/H264VideoRTPSink.cpp
        liveMedia/H264VideoRTPSource.cpp
        liveMedia/H264VideoStreamDiscreteFramer.cpp
        liveMedia/H264VideoStreamFramer.cpp
        liveMedia/H265VideoFileServerMediaSubsession.cpp
        liveMedia/H265VideoFileSink.cpp
        liveMedia/H265VideoRTPSink.cpp
        liveMedia/H265VideoRTPSource.cpp
        liveMedia/H265VideoStreamDiscreteFramer.cpp
        liveMedia/H265VideoStreamFramer.cpp
        liveMedia/InputFile.cpp
        liveMedia/JPEG2000VideoRTPSink.cpp
        liveMedia/JPEG2000VideoRTPSource.cpp
        liveMedia/JPEGVideoRTPSink.cpp
        liveMedia/JPEGVideoRTPSource.cpp
        liveMedia/JPEGVideoSource.cpp
        liveMedia/Locale.cpp
        liveMedia/MatroskaDemuxedTrack.cpp
        liveMedia/MatroskaDemuxedTrack.hh
        liveMedia/MatroskaFile.cpp
        liveMedia/MatroskaFileParser.cpp
        liveMedia/MatroskaFileParser.hh
        liveMedia/MatroskaFileServerDemux.cpp
        liveMedia/MatroskaFileServerMediaSubsession.cpp
        liveMedia/MatroskaFileServerMediaSubsession.hh
        liveMedia/Media.cpp
        liveMedia/MediaSession.cpp
        liveMedia/MediaSink.cpp
        liveMedia/MediaSource.cpp
        liveMedia/MP3ADU.cpp
        liveMedia/MP3ADUdescriptor.cpp
        liveMedia/MP3ADUdescriptor.hh
        liveMedia/MP3ADUinterleaving.cpp
        liveMedia/MP3ADURTPSink.cpp
        liveMedia/MP3ADURTPSource.cpp
        liveMedia/MP3ADUTranscoder.cpp
        liveMedia/MP3AudioFileServerMediaSubsession.cpp
        liveMedia/MP3AudioMatroskaFileServerMediaSubsession.cpp
        liveMedia/MP3AudioMatroskaFileServerMediaSubsession.hh
        liveMedia/MP3FileSource.cpp
        liveMedia/MP3Internals.cpp
        liveMedia/MP3Internals.hh
        liveMedia/MP3InternalsHuffman.cpp
        liveMedia/MP3InternalsHuffman.hh
        liveMedia/MP3InternalsHuffmanTable.cpp
        liveMedia/MP3StreamState.cpp
        liveMedia/MP3StreamState.hh
        liveMedia/MP3Transcoder.cpp
        liveMedia/MPEG1or2AudioRTPSink.cpp
        liveMedia/MPEG1or2AudioRTPSource.cpp
        liveMedia/MPEG1or2AudioStreamFramer.cpp
        liveMedia/MPEG1or2Demux.cpp
        liveMedia/MPEG1or2DemuxedElementaryStream.cpp
        liveMedia/MPEG1or2DemuxedServerMediaSubsession.cpp
        liveMedia/MPEG1or2FileServerDemux.cpp
        liveMedia/MPEG1or2VideoFileServerMediaSubsession.cpp
        liveMedia/MPEG1or2VideoRTPSink.cpp
        liveMedia/MPEG1or2VideoRTPSource.cpp
        liveMedia/MPEG1or2VideoStreamDiscreteFramer.cpp
        liveMedia/MPEG1or2VideoStreamFramer.cpp
        liveMedia/MPEG2IndexFromTransportStream.cpp
        liveMedia/MPEG2TransportFileServerMediaSubsession.cpp
        liveMedia/MPEG2TransportStreamAccumulator.cpp
        liveMedia/MPEG2TransportStreamDemux.cpp
        liveMedia/MPEG2TransportStreamDemuxedTrack.cpp
        liveMedia/MPEG2TransportStreamDemuxedTrack.hh
        liveMedia/MPEG2TransportStreamFramer.cpp
        liveMedia/MPEG2TransportStreamFromESSource.cpp
        liveMedia/MPEG2TransportStreamFromPESSource.cpp
        liveMedia/MPEG2TransportStreamIndexFile.cpp
        liveMedia/MPEG2TransportStreamMultiplexor.cpp
        liveMedia/MPEG2TransportStreamParser.cpp
        liveMedia/MPEG2TransportStreamParser.hh
        liveMedia/MPEG2TransportStreamParser_PAT.cpp
        liveMedia/MPEG2TransportStreamParser_PMT.cpp
        liveMedia/MPEG2TransportStreamParser_STREAM.cpp
        liveMedia/MPEG2TransportStreamTrickModeFilter.cpp
        liveMedia/MPEG2TransportUDPServerMediaSubsession.cpp
        liveMedia/MPEG4ESVideoRTPSink.cpp
        liveMedia/MPEG4ESVideoRTPSource.cpp
        liveMedia/MPEG4GenericRTPSink.cpp
        liveMedia/MPEG4GenericRTPSource.cpp
        liveMedia/MPEG4LATMAudioRTPSink.cpp
        liveMedia/MPEG4LATMAudioRTPSource.cpp
        liveMedia/MPEG4VideoFileServerMediaSubsession.cpp
        liveMedia/MPEG4VideoStreamDiscreteFramer.cpp
        liveMedia/MPEG4VideoStreamFramer.cpp
        liveMedia/MPEGVideoStreamFramer.cpp
        liveMedia/MPEGVideoStreamParser.cpp
        liveMedia/MPEGVideoStreamParser.hh
        liveMedia/MultiFramedRTPSink.cpp
        liveMedia/MultiFramedRTPSource.cpp
        liveMedia/OggDemuxedTrack.cpp
        liveMedia/OggDemuxedTrack.hh
        liveMedia/OggFile.cpp
        liveMedia/OggFileParser.cpp
        liveMedia/OggFileParser.hh
        liveMedia/OggFileServerDemux.cpp
        liveMedia/OggFileServerMediaSubsession.cpp
        liveMedia/OggFileServerMediaSubsession.hh
        liveMedia/OggFileSink.cpp
        liveMedia/OnDemandServerMediaSubsession.cpp
        liveMedia/ourMD5.cpp
        liveMedia/OutputFile.cpp
        liveMedia/PassiveServerMediaSubsession.cpp
        liveMedia/ProxyServerMediaSession.cpp
        liveMedia/QCELPAudioRTPSource.cpp
        liveMedia/QuickTimeFileSink.cpp
        liveMedia/QuickTimeGenericRTPSource.cpp
        liveMedia/RawVideoRTPSink.cpp
        liveMedia/RawVideoRTPSource.cpp
        liveMedia/RTCP.cpp
        liveMedia/rtcp_from_spec.c
        liveMedia/rtcp_from_spec.h
        liveMedia/RTPInterface.cpp
        liveMedia/RTPSink.cpp
        liveMedia/RTPSource.cpp
        liveMedia/RTSPClient.cpp
        liveMedia/RTSPCommon.cpp
        liveMedia/RTSPRegisterSender.cpp
        liveMedia/RTSPServer.cpp
        liveMedia/RTSPServerRegister.cpp
        liveMedia/RTSPServerSupportingHTTPStreaming.cpp
        liveMedia/ServerMediaSession.cpp
        liveMedia/SimpleRTPSink.cpp
        liveMedia/SimpleRTPSource.cpp
        liveMedia/SIPClient.cpp
        liveMedia/StreamParser.cpp
        liveMedia/StreamParser.hh
        liveMedia/StreamReplicator.cpp
        liveMedia/T140TextRTPSink.cpp
        liveMedia/TCPStreamSink.cpp
        liveMedia/TextRTPSink.cpp
        liveMedia/TheoraVideoRTPSink.cpp
        liveMedia/TheoraVideoRTPSource.cpp
        liveMedia/uLawAudioFilter.cpp
        liveMedia/VideoRTPSink.cpp
        liveMedia/VorbisAudioRTPSink.cpp
        liveMedia/VorbisAudioRTPSource.cpp
        liveMedia/VP8VideoRTPSink.cpp
        liveMedia/VP8VideoRTPSource.cpp
        liveMedia/VP9VideoRTPSink.cpp
        liveMedia/VP9VideoRTPSource.cpp
        liveMedia/WAVAudioFileServerMediaSubsession.cpp
        liveMedia/WAVAudioFileSource.cpp
        UsageEnvironment/include/Boolean.hh
        UsageEnvironment/include/HashTable.hh
        UsageEnvironment/include/strDup.hh
        UsageEnvironment/include/UsageEnvironment.hh
        UsageEnvironment/include/UsageEnvironment_version.hh
        UsageEnvironment/HashTable.cpp
        UsageEnvironment/strDup.cpp
        UsageEnvironment/UsageEnvironment.cpp
	testRTSPClient.cpp) # main client file

# target linking HELLO_PI_LIBS for the video.c file
target_link_libraries(fprotocol_client ${HELLO_PI_LIBS})
