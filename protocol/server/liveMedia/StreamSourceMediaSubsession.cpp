//
// Created by vm1 on 10/19/2019.
//

#include <include/DeviceSource.hh>
#include <include/H264VideoStreamDiscreteFramer.hh>
#include <include/H264VideoRTPSink.hh>
#include "StreamSourceMediaSubsession.h"

StreamSourceMediaSubsession::StreamSourceMediaSubsession(UsageEnvironment &env, Boolean reuseFirstSource): OnDemandServerMediaSubsession(env, reuseFirstSource) {

}

StreamSourceMediaSubsession::~StreamSourceMediaSubsession() {

}

FramedSource *StreamSourceMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate) {
    static DeviceSource* source = DeviceSource::createNew(envir()); // Create a framer for the Video Elementary Stream:
    //return H264VideoStreamFramer::createNew(envir(), source);
    return H264VideoStreamDiscreteFramer::createNew(envir(), source);
}

RTPSink *StreamSourceMediaSubsession::createNewRTPSink(Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
                                                       FramedSource *inputSource) {
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

StreamSourceMediaSubsession *
StreamSourceMediaSubsession::createNew(UsageEnvironment &env, Boolean reuseFirstSource) {
    return new StreamSourceMediaSubsession(env, reuseFirstSource);
}
