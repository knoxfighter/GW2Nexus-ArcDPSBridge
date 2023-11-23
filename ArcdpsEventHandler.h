#pragma once

#include "ArcdpsExtension/EventSequencer.h"
#include "ArcdpsExtension/Singleton.h"
#include "ArcdpsExtension/arcdps_structs_slim.h"
#include "Nexus/Nexus.h"

class EventSequencer {
public:
    EventSequencer(const char* pEventId) : mEventId(pEventId) {
        using namespace std::chrono_literals;
        mThread = std::jthread([this](std::stop_token stoken) {
            while(!stoken.stop_requested()) {
                while(!stoken.stop_requested() && !mElements.empty()) {
                    Runner();
                }

                if (stoken.stop_requested()) return;

                std::this_thread::sleep_for(100ms);
            }
        });
        mThread.detach();
    }
    virtual ~EventSequencer() {
        if (mThread.joinable()) {
            mThread.request_stop();
            mThread.join();
        }
    }

    // delete copy and move
    EventSequencer(const EventSequencer& pOther) = delete;
    EventSequencer(EventSequencer&& pOther) noexcept = delete;
    EventSequencer& operator=(const EventSequencer& pOther) = delete;
    EventSequencer& operator=(EventSequencer&& pOther) noexcept = delete;

    struct Event {
        struct : cbtevent {
            bool Present = false;
        } Ev;

        struct : ag {
            std::string NameStorage;
            bool Present = false;
        } Source;

        struct : ag {
            std::string NameStorage;
            bool Present = false;
        } Destination;

        const char* Skillname; // Skill names are guaranteed to be valid for the lifetime of the process so copying pointer is fine
        uint64_t Id;
        uint64_t Revision;

        std::strong_ordering operator<=>(const Event& pOther) const {
            return Id <=> pOther.Id;
        }

        Event(cbtevent* pEv, ag* pSrc, ag* pDst, const char* pSkillname, uint64_t pId, uint64_t pRevision)
            : Skillname(pSkillname),
              Id(pId),
              Revision(pRevision) {
            if (pEv) {
                *static_cast<cbtevent*>(&Ev) = *pEv;
                Ev.Present = true;
            }
            if (pSrc) {
                *static_cast<ag*>(&Source) = *pSrc;
                Source.Present = true;
                if (Source.name) {
                    Source.NameStorage = pSrc->name;
                }
            }
            if (pDst) {
                *static_cast<ag*>(&Destination) = *pDst;
                Destination.Present = true;
                if (Destination.name) {
                    Destination.NameStorage = pDst->name;
                }
            }
        }
    };

    void SetAddonApi(AddonAPI* pAddonApi) {
        mAddonApi = pAddonApi;
    }

    void ProcessEvent(cbtevent* pEv, ag* pSrc, ag* pDst, const char* pSkillname, uint64_t pId, uint64_t pRevision) {
        std::lock_guard guard(mElementsMutex);
        if (pId == 0) {
            if (mElements.empty()) {
                Event event(pEv, pSrc, pDst, pSkillname, pId, pRevision);
                mAddonApi->RaiseEvent(mEventId, &event);
//                mCallback(pEv, pSrc, pDst, pSkillname, pId, pRevision);
                // mPriorityElements.emplace(pEv, pSrc, pDst, pSkillname, mNextId, pRevision);
                return;
            }

            // insert it in the prio queue
            mElements.emplace(pEv, pSrc, pDst, pSkillname, mLastId, pRevision);
        } else {
            mLastId = pId;
            mElements.emplace(pEv, pSrc, pDst, pSkillname, pId, pRevision);
        }
    }

private:
    AddonAPI* mAddonApi = nullptr;
    const char* mEventId;
    std::multiset<Event> mElements;
    std::mutex mElementsMutex;
    std::jthread mThread;
    uint64_t mNextId = 2; // Events start with ID 2 for some reason (it is always like that and no plans to change)
    uint64_t mLastId = 2;
    bool mThreadRunning = false;

    template<bool First = true>
    void Runner() {
        std::unique_lock guard(mElementsMutex);

        if (!mElements.empty() && mElements.begin()->Id == mNextId && mAddonApi) {
            mThreadRunning = true;
            auto item = mElements.extract(mElements.begin());
            guard.unlock();
            mAddonApi->RaiseEvent(mEventId, &item.value());
            mThreadRunning = false;
            Runner<false>();

            if constexpr (First) {
                ++mNextId;
            }
        }
    }
};
