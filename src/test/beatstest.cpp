#include <gtest/gtest.h>

#include <QtDebug>

#include "track/beats.h"
#include "track/track.h"
#include "util/memory.h"

using namespace mixxx;

namespace {

class BeatsTest : public testing::Test {
  protected:
    BeatsTest()
            : m_pTrack(Track::newTemporary()),
              m_iChannelCount(2),
              m_iSampleRate(44100),
              m_framesPerSecond(m_iSampleRate / m_iChannelCount),
              m_frameSize(m_iChannelCount),
              m_pBeats1(new Beats(m_pTrack.get())),
              m_pBeats2(new Beats(m_pTrack.get())),
              m_bpm(60),
              m_startOffsetFrames(7),
              m_beatLengthFrames(getBeatLength(m_bpm)) {
        m_pTrack->setAudioProperties(
                mixxx::audio::ChannelCount(m_iChannelCount),
                mixxx::audio::SampleRate(m_iSampleRate),
                mixxx::audio::Bitrate(),
                mixxx::Duration::fromSeconds(180));
        m_pBeats1->setGrid(m_bpm, Frame(m_startOffsetFrames));
        m_pBeats2->setGrid(m_bpm, Frame(m_startOffsetFrames));
        m_firstBeat = m_pBeats1->getFirstBeatPosition();
        m_lastBeat = m_pBeats1->getLastBeatPosition();
    }

    ~BeatsTest() {
    }

    Frame getBeatLength(Bpm bpm) {
        if (bpm == Bpm()) {
            DEBUG_ASSERT(false);
            return Frame();
        }
        return Frame((60.0 * m_framesPerSecond.getValue() / bpm.getValue()));
    }

    QVector<Frame> createBeatVector(Frame first_beat,
            unsigned int num_beats,
            Frame beat_length) {
        QVector<Frame> beats;
        for (unsigned int i = 0; i < num_beats; ++i) {
            beats.append(first_beat + beat_length * i);
        }
        return beats;
    }

    TrackPointer m_pTrack;
    const int m_iChannelCount;
    // Sample Rate is a standard unit
    const SINT m_iSampleRate;
    // We internally use frames per second since a frame position
    // actually matters when calculating beats.
    const Frame m_framesPerSecond;
    const Frame m_frameSize;
    BeatsPointer m_pBeats1;
    BeatsPointer m_pBeats2;
    const Bpm m_bpm;
    const Frame m_startOffsetFrames;
    const Frame m_beatLengthFrames;
    Frame m_firstBeat;
    Frame m_lastBeat;
};

TEST_F(BeatsTest, Scale) {
    // Initially must be the base value
    EXPECT_EQ(m_bpm, m_pBeats1->getBpm());

    m_pBeats1->scale(Beats::DOUBLE);
    EXPECT_EQ(m_bpm * 2, m_pBeats1->getBpm());

    m_pBeats1->scale(Beats::HALVE);
    EXPECT_EQ(m_bpm, m_pBeats1->getBpm());

    m_pBeats1->scale(Beats::TWOTHIRDS);
    EXPECT_EQ(m_bpm * 2 / 3, m_pBeats1->getBpm());

    m_pBeats1->scale(Beats::THREEHALVES);
    EXPECT_EQ(m_bpm, m_pBeats1->getBpm());

    m_pBeats1->scale(Beats::THREEFOURTHS);
    EXPECT_EQ(m_bpm * 3 / 4, m_pBeats1->getBpm());

    m_pBeats1->scale(Beats::FOURTHIRDS);
    EXPECT_EQ(m_bpm, m_pBeats1->getBpm());
}

TEST_F(BeatsTest, NthBeat) {
    // Check edge cases
    EXPECT_EQ(m_lastBeat, m_pBeats1->findNthBeat(m_lastBeat, 1));
    EXPECT_EQ(m_lastBeat, m_pBeats1->findNextBeat(m_lastBeat));
    EXPECT_EQ(-1, m_pBeats1->findNthBeat(m_lastBeat, 2).getValue());
    EXPECT_EQ(m_firstBeat, m_pBeats1->findNthBeat(m_firstBeat, -1));
    EXPECT_EQ(m_firstBeat, m_pBeats1->findPrevBeat(m_firstBeat));
    EXPECT_EQ(-1, m_pBeats1->findNthBeat(m_firstBeat, -2).getValue());

    // TODO(JVC) Add some tests in the middle
}

TEST_F(BeatsTest, PrevNextBeats) {
    Frame prevBeat, nextBeat;

    m_pBeats1->findPrevNextBeats(m_lastBeat, &prevBeat, &nextBeat);
    EXPECT_DOUBLE_EQ(m_lastBeat.getValue(), prevBeat.getValue());
    EXPECT_DOUBLE_EQ(-1, nextBeat.getValue());

    m_pBeats1->findPrevNextBeats(m_firstBeat, &prevBeat, &nextBeat);
    EXPECT_DOUBLE_EQ(m_firstBeat.getValue(), prevBeat.getValue());
    EXPECT_DOUBLE_EQ((m_firstBeat + m_beatLengthFrames).getValue(), nextBeat.getValue());

    // TODO(JVC) Add some tests in the middle
}

TEST_F(BeatsTest, NthBeatWhenOnBeat) {
    // Pretend we're on the 20th beat;
    const int curBeat = 20;
    Frame position = m_startOffsetFrames + Frame(m_beatLengthFrames.getValue() * curBeat);

    // The spec dictates that a value of 0 is always invalid and returns -1
    EXPECT_EQ(-1, m_pBeats1->findNthBeat(position, 0).getValue());

    // findNthBeat should return exactly the current beat if we ask for 1 or
    // -1. For all other values, it should return n times the beat length.
    for (int i = 1; i < curBeat; ++i) {
        EXPECT_DOUBLE_EQ(
                (position + Frame(m_beatLengthFrames.getValue() * (i - 1))).getValue(),
                m_pBeats1->findNthBeat(position, i).getValue());
        EXPECT_DOUBLE_EQ(
                (position + Frame(m_beatLengthFrames.getValue() * (-i + 1))).getValue(),
                m_pBeats1->findNthBeat(position, -i).getValue());
    }

    // Also test prev/next beat calculation.
    Frame prevBeat, nextBeat;
    m_pBeats1->findPrevNextBeats(position, &prevBeat, &nextBeat);
    EXPECT_EQ(position, prevBeat);
    EXPECT_EQ(position + m_beatLengthFrames, nextBeat);

    // Both previous and next beat should return the current position.
    EXPECT_EQ(position, m_pBeats1->findNextBeat(position));
    EXPECT_EQ(position, m_pBeats1->findPrevBeat(position));
}

TEST_F(BeatsTest, NthBeatWhenOnBeat_BeforeEpsilon) {
    // Pretend we're just before the 20th beat;
    const int curBeat = 20;
    const Frame kClosestBeat = m_startOffsetFrames + Frame(curBeat * m_beatLengthFrames.getValue());
    Frame position = kClosestBeat - Frame(m_beatLengthFrames.getValue() * 0.005);

    // The spec dictates that a value of 0 is always invalid and returns -1
    EXPECT_EQ(Frame(-1), m_pBeats1->findNthBeat(position, 0));

    // findNthBeat should return exactly the current beat if we ask for 1 or
    // -1. For all other values, it should return n times the beat length.
    for (int i = 1; i < curBeat; ++i) {
        EXPECT_DOUBLE_EQ((kClosestBeat + Frame(m_beatLengthFrames.getValue() * (i - 1))).getValue(),
                m_pBeats1->findNthBeat(position, i).getValue());
        EXPECT_DOUBLE_EQ(
                (kClosestBeat + Frame(m_beatLengthFrames.getValue() * (-i + 1)))
                        .getValue(),
                m_pBeats1->findNthBeat(position, -i).getValue());
    }

    // Also test prev/next beat calculation
    Frame prevBeat, nextBeat;
    m_pBeats1->findPrevNextBeats(position, &prevBeat, &nextBeat);
    EXPECT_EQ(kClosestBeat, prevBeat);
    EXPECT_EQ(kClosestBeat + m_beatLengthFrames, nextBeat);

    // Both previous and next beat should return the closest beat.
    EXPECT_EQ(kClosestBeat, m_pBeats1->findNextBeat(position));
    EXPECT_EQ(kClosestBeat, m_pBeats1->findPrevBeat(position));
}

TEST_F(BeatsTest, NthBeatWhenOnBeat_AfterEpsilon) {
    // Pretend we're just after the 20th beat;
    const int curBeat = 20;
    const Frame kClosestBeat = m_startOffsetFrames +
            Frame(curBeat * m_beatLengthFrames.getValue());
    Frame position =
            kClosestBeat + Frame(m_beatLengthFrames.getValue() * 0.005);

    // The spec dictates that a value of 0 is always invalid and returns -1
    EXPECT_EQ(Frame(-1), m_pBeats1->findNthBeat(position, 0));

    EXPECT_EQ(kClosestBeat, m_pBeats1->findClosestBeat(position));

    // findNthBeat should return exactly the current beat if we ask for 1 or
    // -1. For all other values, it should return n times the beat length.
    for (int i = 1; i < curBeat; ++i) {
        EXPECT_DOUBLE_EQ(
                (kClosestBeat + Frame(m_beatLengthFrames.getValue() * (i - 1)))
                        .getValue(),
                m_pBeats1->findNthBeat(position, i).getValue());
        EXPECT_DOUBLE_EQ(
                (kClosestBeat + Frame(m_beatLengthFrames.getValue() * (-i + 1)))
                        .getValue(),
                m_pBeats1->findNthBeat(position, -i).getValue());
    }

    // Also test prev/next beat calculation.
    Frame prevBeat, nextBeat;
    m_pBeats1->findPrevNextBeats(position, &prevBeat, &nextBeat);
    EXPECT_EQ(kClosestBeat, prevBeat);
    EXPECT_EQ(kClosestBeat + m_beatLengthFrames, nextBeat);

    // Both previous and next beat should return the closest beat.
    EXPECT_EQ(kClosestBeat, m_pBeats1->findNextBeat(position));
    EXPECT_EQ(kClosestBeat, m_pBeats1->findPrevBeat(position));
}

TEST_F(BeatsTest, NthBeatWhenNotOnBeat) {
    // Pretend we're half way between the 20th and 21st beat
    Frame previousBeat =
            m_startOffsetFrames + m_beatLengthFrames * 20.0;
    Frame nextBeat =
            m_startOffsetFrames + m_beatLengthFrames * 21.0;
    Frame position = (nextBeat + previousBeat) / 2.0;

    // The spec dictates that a value of 0 is always invalid and returns -1
    EXPECT_EQ(Frame(-1), m_pBeats1->findNthBeat(position, 0));

    // findNthBeat should return multiples of beats starting from the next or
    // previous beat, depending on whether N is positive or negative.
    for (int i = 1; i < 20; ++i) {
        EXPECT_DOUBLE_EQ(
                (nextBeat + m_beatLengthFrames * (i - 1))
                        .getValue(),
                m_pBeats1->findNthBeat(position, i).getValue());
        EXPECT_DOUBLE_EQ(
                (nextBeat + m_beatLengthFrames * (-i + 1))
                        .getValue(),
                m_pBeats1->findNthBeat(position, -i).getValue());
    }

    // Also test prev/next beat calculation
    Frame foundPrevBeat, foundNextBeat;
    m_pBeats1->findPrevNextBeats(position, &foundPrevBeat, &foundNextBeat);
    EXPECT_EQ(previousBeat, foundPrevBeat);
    EXPECT_EQ(nextBeat, foundNextBeat);
}

TEST_F(BeatsTest, BpmAround) {
    Frame approx_beat_length = getBeatLength(m_bpm);
    const int numBeats = 64;

    // Constant BPM, constructed in BeatsTest
    for (unsigned int i = 0; i < 100; i++) {
        EXPECT_EQ(Bpm(60), m_pBeats1->getBpmAroundPosition(Frame(i), 5));
    }

    // Prepare a new Beats to test the behavior for variable BPM
    QVector<Frame> beats;
    Frame beat_pos;
    Bpm bpm(60);
    for (unsigned int i = 0; i < numBeats; ++i, bpm = bpm + 1) {
        Frame beat_length = getBeatLength(bpm);
        beats.append(beat_pos);
        beat_pos += beat_length;
    }
    BeatsPointer pMap =
            std::make_unique<Beats>(m_pTrack.get(), beats);

    // The average of the first 8 beats should be different than the average
    // of the last 8 beats.
    EXPECT_DOUBLE_EQ(64.024390243902445,
            pMap->getBpmAroundPosition(approx_beat_length * 4, 4).getValue());
    EXPECT_DOUBLE_EQ(118.98016997167139,
            pMap->getBpmAroundPosition(approx_beat_length * 60, 4).getValue());
    // Also test at the beginning and end of the track
    EXPECT_DOUBLE_EQ(62.968515742128936,
            pMap->getBpmAroundPosition(Frame(0), 4).getValue());
    EXPECT_DOUBLE_EQ(118.98016997167139,
            pMap->getBpmAroundPosition(approx_beat_length * 65, 4).getValue());

    // Try a really, really short track
    beats = createBeatVector(Frame(10), 3, getBeatLength(bpm));
    BeatsPointer pBeats =
            std::make_unique<Beats>(m_pTrack.get(), beats);
    EXPECT_DOUBLE_EQ(bpm.getValue(),
            pMap->getBpmAroundPosition(approx_beat_length * 1, 4).getValue());
}

TEST_F(BeatsTest, Signature) {
    // Undefined time signature must be default
    EXPECT_EQ(m_pBeats1->getSignature(),
            kDefaultTimeSignature)
            << "If no Time Signature defined, it must be default(4/4)";

    // Add time signature to the beginning
    m_pBeats1->setSignature(TimeSignature(3, 4));

    // Add time signature in beats not at the beginning
    m_pBeats1->setSignature(TimeSignature(5, 4), Frame(1000000));
    m_pBeats1->setSignature(TimeSignature(5, 3), Frame(5000000));

    TimeSignature test = m_pBeats1->getSignature();
    EXPECT_EQ(m_pBeats1->getSignature(),
            TimeSignature(3, 4))
            << "Starting Time Signature must be 3/4";
    EXPECT_EQ(m_pBeats1->getSignature(Frame(500000)),
            TimeSignature(3, 4))
            << "Time Signature at 500000 must be 3/4";
    EXPECT_EQ(m_pBeats1->getSignature(Frame(1000000)),
            TimeSignature(5, 4))
            << "Time Signature at 1000000 must be 5/4";
    EXPECT_EQ(m_pBeats1->getSignature(Frame(5000000)),
            TimeSignature(5, 3))
            << "Time Signature at 5000000 must be 5/3";
    EXPECT_EQ(m_pBeats1->getSignature(Frame(100000000)),
            TimeSignature(5, 3))
            << "Time Signature at 100000000 must be 5/3";

    // Add a signature past the end of the track, must have no effect, and check
    m_pBeats1->setSignature(TimeSignature(6, 4), Frame(10000000));
    EXPECT_EQ(m_pBeats1->getSignature(Frame(100000000)),
            TimeSignature(5, 3))
            << "setSignature after the end of track must have no effect";
}

// TODO(XXX) During testing some situations where calling isBar where generating
// a SIGSEV where found. This simply crashes the test. Can be tested in a more
// elegant way?
TEST_F(BeatsTest, Iterator) {
    Frame pos;

    // Full Beatsbeat
    auto iter1 = m_pBeats1->findBeats(m_pBeats1->getFirstBeatPosition(),
            m_pBeats1->getLastBeatPosition());
    EXPECT_DOUBLE_EQ(iter1->next(), m_pBeats1->getFirstBeatPosition().getValue());
    while (iter1->hasNext()) {
        pos = Frame(iter1->next());
        EXPECT_TRUE(pos.getValue());
        iter1->isBar();
    }
    EXPECT_DOUBLE_EQ(pos.getValue(), m_pBeats1->getLastBeatPosition().getValue());

    // Past end
    auto iter2 = m_pBeats1->findBeats(m_pBeats1->getFirstBeatPosition(),
            Frame(m_pBeats1->getLastBeatPosition().getValue() + 10000000000));
    while (iter2->hasNext()) {
        pos = Frame(iter2->next());
        EXPECT_TRUE(pos.getValue());
        iter2->isBar();
    }
    EXPECT_DOUBLE_EQ(pos.getValue(), m_pBeats1->getLastBeatPosition().getValue());

    // Before begining
    auto iter3 = m_pBeats1->findBeats(Frame(m_pBeats1->getFirstBeatPosition().getValue() - 1000000),
            m_pBeats1->getLastBeatPosition());
    EXPECT_DOUBLE_EQ(iter3->next(), m_pBeats1->getFirstBeatPosition().getValue());
    while (iter3->hasNext()) {
        pos = Frame(iter3->next());
        EXPECT_TRUE(pos.getValue());
        iter3->isBar();
    }
    EXPECT_DOUBLE_EQ(pos.getValue(), m_pBeats1->getLastBeatPosition().getValue());
}

TEST_F(BeatsTest, Translate) {
    Frame delta = Frame(500);

    // Move the grid delta frames
    m_pBeats1->translate(delta);

    // All beats must have been displaced by delta frames
    auto iter1 = m_pBeats1->findBeats(m_pBeats1->getFirstBeatPosition(),
            m_pBeats1->getLastBeatPosition());
    auto iter2 = m_pBeats2->findBeats(m_pBeats2->getFirstBeatPosition(),
            m_pBeats2->getLastBeatPosition());
    while (iter1->hasNext()) {
        double pos1 = iter1->next();
        double pos2 = iter2->next();
        EXPECT_DOUBLE_EQ(pos1, pos2 + delta.getValue());
    }
    EXPECT_EQ(iter1->hasNext(), iter2->hasNext());
}

TEST_F(BeatsTest, FindClosest) {
    // Test deltas ranging from previous beat to next beat
    for (Frame delta = Frame(-m_iSampleRate); delta <= Frame(m_iSampleRate); delta++) {
        auto iter1 = m_pBeats1->findBeats(m_pBeats1->getFirstBeatPosition(),
                m_pBeats1->getLastBeatPosition());
        while (iter1->hasNext()) {
            Frame pos = Frame(iter1->next());
            Frame foundPos = m_pBeats1->findClosestBeat(pos + delta);
            // Correct change of beat
            Frame expectedPos = pos +
                    Frame((delta.getValue() > m_iSampleRate / 2 ? m_iSampleRate : 0)) +
                    Frame((delta.getValue() < -m_iSampleRate / 2 ? -m_iSampleRate : 0));
            // Enforce boundaries
            expectedPos = std::min(expectedPos, m_pBeats1->getLastBeatPosition());
            expectedPos = std::max(expectedPos, m_pBeats1->getFirstBeatPosition());
            EXPECT_DOUBLE_EQ(foundPos.getValue(), expectedPos.getValue());
        }
        break;
    }
}

} // namespace
