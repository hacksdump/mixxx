#pragma once

#include <QByteArray>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

#include "proto/beats.pb.h"
#include "track/bpm.h"
#include "track/frame.h"
#include "util/types.h"

namespace mixxx {

class Beats;
class BeatIterator;
using BeatsPointer = std::shared_ptr<Beats>;
using BeatList = QList<track::io::Beat>;
} // namespace mixxx

#include "track/beatiterator.h"
#include "track/timesignature.h"

class Track;

namespace mixxx {
/// This is an intermediate class which encapsulates the beats into a
/// plain copyable, movable object.
class BeatsInternal {
  public:
    // TODO(hacksdump): These versions are retained for backward compatibility.
    // In future, There will be no versions at all.
    static const QString BEAT_MAP_VERSION;
    static const QString BEAT_GRID_1_VERSION;
    static const QString BEAT_GRID_2_VERSION;
    BeatsInternal();
    FramePos findNthBeat(FramePos frame, int offset) const;
    Bpm getBpm() const;
    bool isValid() const;
    void setSampleRate(int sampleRate) {
        m_iSampleRate = sampleRate;
    }
    void setDurationSeconds(double duration) {
        m_dDurationSeconds = duration;
    }
    int numBeatsInRange(FramePos startFrame, FramePos endFrame) const;
    QByteArray toProtobuf() const;
    QString getVersion() const;
    QString getSubVersion() const;
    void setSubVersion(const QString& subVersion);
  private:
    QString m_subVersion;
    Bpm m_bpm;
    BeatList m_beats;
    int m_iSampleRate;
    double m_dDurationSeconds;
    friend class Beats;
    friend QDebug operator<<(QDebug dbg, const BeatsInternal& arg);
};

/// Beats is a class for BPM and beat management classes.
/// It stores beats information including beats position, down beats position,
/// phrase beat position and changes in tempo.
class Beats final : public QObject {
    Q_OBJECT
  public:
    /// Initialize beats with only the track pointer.
    explicit Beats(const Track* track);
    /// The source of this byte array is the serialized representation of beats
    /// generated by the protocol buffer and stored in the database.
    Beats(const Track* track, const QByteArray& byteArray);
    /// A list of beat locations in audio frames may be provided.
    /// The source of this data is the analyzer.
    Beats(const Track* track, const QVector<FramePos>& beats);
    ~Beats() override = default;

    using iterator = BeatIterator;

    // TODO(JVC) Is a copy constructor needed? of we can force a move logic??
    Beats(const mixxx::Beats& other);

    enum BPMScale {
        DOUBLE,
        HALVE,
        TWOTHIRDS,
        THREEFOURTHS,
        FOURTHIRDS,
        THREEHALVES,
    };

    /// Serializes into a protobuf.
    QByteArray toProtobuf() const;
    BeatsPointer clone() const;

    /// Returns a string representing the version of the beat-processing code that
    /// produced this Beats instance. Used by BeatsFactory for associating a
    /// given serialization with the version that produced it.
    QString getVersion() const;
    /// Return a string that represent the preferences used to generate
    /// the beats object.
    QString getSubVersion() const;
    void setSubVersion(QString subVersion);
    bool isValid() const;
    /// Calculates the BPM between two beat positions.
    Bpm calculateBpm(const track::io::Beat& startBeat,
            const track::io::Beat& stopBeat) const;

    /// Initializes the BeatGrid to have a BPM of dBpm and the first beat offset
    /// of firstBeatFrame. Does not generate an updated() signal, since it is
    /// meant for initialization.
    void setGrid(Bpm dBpm, FramePos firstBeatFrame = FramePos());

    // TODO: We may want to implement these with common code that returns
    //       the triple of closest, next, and prev.

    /// Starting from frame, return the frame number of the next beat
    /// in the track, or -1 if none exists. If frame refers to the location
    /// of a beat, frame is returned.
    FramePos findNextBeat(FramePos frame) const;

    /// Starting from frame frame, return the frame number of the previous
    /// beat in the track, or -1 if none exists. If frame refers to the
    /// location of beat, frame is returned.
    FramePos findPrevBeat(FramePos frame) const;

    /// Starting from frame, fill the frame numbers of the previous beat
    /// and next beat.  Either can be -1 if none exists.  If frame refers
    /// to the location of the beat, the first value is frame, and the second
    /// value is the next beat position.  Non- -1 values are guaranteed to be
    /// even.  Returns false if *at least one* frame is -1.  (Can return false
    /// with one beat successfully filled)
    bool findPrevNextBeats(FramePos frame,
            FramePos* pPrevBeatFrame,
            FramePos* pNextBeatFrame) const;

    /// Starting from frame, return the frame number of the closest beat
    /// in the track, or -1 if none exists.  Non- -1 values are guaranteed to be
    /// even.
    FramePos findClosestBeat(FramePos frame) const;

    /// Find the Nth beat from frame. Works with both positive and
    /// negative values of n. If frame refers to the location of a beat,
    /// then frame is returned. If no beat can be found, returns -1.
    FramePos findNthBeat(FramePos frame, int offset) const;

    int numBeatsInRange(FramePos startFrame, FramePos endFrame);

    /// Find the frame N beats away from frame. The number of beats may be
    /// negative and does not need to be an integer.
    FramePos findNBeatsFromFrame(FramePos fromFrame, double beats) const;

    /// Return an iterator to a container of Beats containing the Beats
    /// between startFrameNum and endFrameNum. THe BeatIterator must be iterated
    /// while a strong reference to the Beats object to ensure that the Beats
    /// object is not deleted. Caller takes ownership of the returned BeatsIterator
    std::unique_ptr<Beats::iterator> findBeats(FramePos startFrame,
            FramePos stopFrame) const;

    /// Return whether or not a Beat lies between startFrameNum and endFrameNum
    bool hasBeatInRange(FramePos startFrame,
            FramePos stopFrame) const;

    /// Return the average BPM over the entire track if the BPM is
    /// valid, otherwise returns -1
    Bpm getBpm() const;

    /// Return the average BPM over the range from startFrameNum to endFrameNum,
    /// specified in frames if the BPM is valid, otherwise returns -1
    double getBpmRange(FramePos startFrame,
            FramePos stopFrame) const;

    /// Return the average BPM over the range of n*2 beats centered around
    /// curFrameNum.  (An n of 4 results in an averaging of 8 beats).  Invalid
    /// BPM returns -1.
    Bpm getBpmAroundPosition(FramePos curFrame, int n) const;

    /// Sets the track signature at the nearest frame
    void setSignature(TimeSignature signature, FramePos frame = FramePos());

    /// Return the track signature at the given frame position
    TimeSignature getSignature(FramePos frame = FramePos()) const;

    /// Sets the nearest beat as a downbeat
    void setDownBeat(FramePos frame = FramePos());

    /// Translate all beats in the song by numFrames. Beats that lie
    /// before the start of the track or after the end of the track are not
    /// removed.
    void translate(FrameDiff_t numFrames);

    /// Scale the position of every beat in the song by dScalePercentage.
    void scale(enum BPMScale scale);

    /// Adjust the beats so the global average BPM matches dBpm.
    void setBpm(Bpm dBpm);

    /// Returns the number of beats
    inline int size() {
        return m_beatsInternal.m_beats.size();
    }

    /// Returns the frame number for the first beat, -1 is no beats
    FramePos getFirstBeatPosition() const;

    /// Returns the frame number for the last beat, -1 if no beats
    FramePos getLastBeatPosition() const;

    /// Return the sample rate
    SINT getSampleRate() const;

    /// Prints debugging information in stderr
    friend QDebug operator<<(QDebug dbg, const BeatsPointer& arg);

  private slots:
    void slotTrackBeatsUpdated();
  private:
    void updateBpm();
    void scaleDouble();
    void scaleTriple();
    void scaleQuadruple();
    void scaleHalve();
    void scaleThird();
    void scaleFourth();
    void scaleMultiple(uint multiple);
    void scaleFraction(uint fraction);

    mutable QMutex m_mutex;
    const Track* m_track;
    BeatsInternal m_beatsInternal;
  signals:
    void updated();
};
} // namespace mixxx
