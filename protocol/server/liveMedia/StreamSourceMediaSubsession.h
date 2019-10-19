//
// Created by vm1 on 10/19/2019.
//

#ifndef PROTOCOL_WINDOWS_STREAMSOURCEMEDIASUBSESSION_H
#define PROTOCOL_WINDOWS_STREAMSOURCEMEDIASUBSESSION_H

#include <OnDemandServerMediaSubsession.hh>
class StreamSourceMediaSubsession: public OnDemandServerMediaSubsession {
public:
    static StreamSourceMediaSubsession*
    createNew(UsageEnvironment& env, Boolean reuseFirstSource);
protected: // we're a virtual base class
    StreamSourceMediaSubsession(UsageEnvironment& env,
            Boolean reuseFirstSource = true);
    virtual ~StreamSourceMediaSubsession();

    FramedSource* createNewStreamSource(unsigned clientSessionId,
                                                unsigned& estBitrate) override;

    RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource* inputSource) override;
};


#endif //PROTOCOL_WINDOWS_STREAMSOURCEMEDIASUBSESSION_H
