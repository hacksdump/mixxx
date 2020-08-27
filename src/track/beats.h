#pragma once

#include <QByteArray>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

#include "audio/streaminfo.h"
#include "proto/beats.pb.h"
#include "track/beat.h"
#include "track/bpm.h"
#include "track/frame.h"
#include "util/types.h"

namespace mixxx {

class Beats;
class BeatIterator;
using BeatsPointer = std::shared_ptr<Beats>;
} // namespace mixxx

#include "track/beatiterator.h"
#include "track/timesignature.h"

class Track;

namespace mixxx {
/// This is an intermediate class which encapsulates the beats into a
/// plain copyable, movable object.
class BeatsInternal {
  public:
    BeatsInternal(const audio::StreamInfo& streamInfo = audio::StreamInfo());
    void initWithProtobuf(const QByteArray& byteArray);
    void initWithAnalyzer(const QVector<FramePos>& beats,
            const QVector<track::io::TimeSignatureMarker>&
                    timeSignatureMarkers =
                            QVector<track::io::TimeSignatureMarker>());

    enum BPMScale {
        DOUBLE,
        HALVE,
        TWOTHIRDS,
        THREEFOURTHS,
        FOURTHIRDS,
        THREEHALVES,
    };

    using iterator = BeatIterator;

    static const QString BEAT_MAP_VERSION;
    static const QString BEAT_GRID_1_VERSION;
    static const QString BEAT_GRID_2_VERSION;
    static const QString BEATS_VERSION;
    Beat findNthBeat(FramePos frame, int offset) const;
    Beat findNextBeat(FramePos frame) const;
    Beat findPrevBeat(FramePos frame) const;
    Bpm getGlobalBpm() const;
    bool isValid() const;
    void updateStreamInfo(const mixxx::audio::StreamInfo& streamInfo);
    int numBeatsInRange(FramePos startFrame, FramePos endFrame) const;
    QByteArray toProtobuf() const;
    QString getVersion() const;
    QString getSubVersion() const;
    void setSubVersion(const QString& subVersion);
    Bpm calculateBpm(const Beat& startBeat,
            const Beat& stopBeat) const;
    void scale(enum BPMScale scale);
    FramePos findNBeatsFromFrame(FramePos fromFrame, double beats) const;
    bool findPrevNextBeats(FramePos frame,
            FramePos* pPrevBeatFrame,
            FramePos* pNextBeatFrame) const;
    void setGrid(Bpm dBpm, FramePos firstBeatFrame = kStartFramePos);
    FramePos findClosestBeat(FramePos frame) const;
    std::unique_ptr<BeatsInternal::iterator> findBeats(
            FramePos startFrame, FramePos stopFrame) const;
    Bpm getBpmAroundPosition(FramePos curFrame, int n) const;
    void setSignature(TimeSignature sig, int downbeatIndex);
    void translate(FrameDiff_t numFrames);
    void setBpm(Bpm bpm, int beatIndex = 0);
    int size() const;
    FramePos getFirstBeatPosition() const;
    FramePos getLastBeatPosition() const;
    Beat getBeatAtIndex(int index) const;
    void setAsDownbeat(int beatIndex);

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
    void generateBeatsFromMarkers();
    void clearMarkers();
    SINT getSampleRate() const;
    double getDurationSeconds() const;

    QString m_subVersion;
    Bpm m_bpm;
    BeatList m_beats;
    track::io::Beats m_beatsProto;
    audio::StreamInfo m_streamInfo;
    friend QDebug operator<<(QDebug dbg, const BeatsInternal& arg);
};

/// Beats is a class for BPM and beat management classes.
/// It stores beats information including beats position, down beats position,
/// phrase beat position and changes in tempo.
class Beats final : public QObject {
    Q_OBJECT
  private:
    explicit Beats(const audio::StreamInfo& streamInfo = audio::StreamInfo(),
            const BeatsInternal& internal = BeatsInternal());

  public:
    ~Beats() override = default;
    Beats(const Beats& other) = delete;

    using iterator = BeatIterator;

    /// The source of this byte array is the serialized representation of beats
    /// generated by the protocol buffer and stored in the database.
    void initWithProtobuf(const QByteArray& byteArray);
    /// A list of beat locations in audio frames may be provided.
    /// The source of this data is the analyzer.
    void initWithAnalyzer(const QVector<FramePos>& beats,
            const QVector<track::io::TimeSignatureMarker>&
                    timeSignatureMarkers =
                            QVector<track::io::TimeSignatureMarker>());

    /// Serializes into a protobuf.
    QByteArray toProtobuf() const;

    /// Returns a string representing the version of the beat-processing code that
    /// produced this Beats instance. Used by BeatsFactory for associating a
    /// given serialization with the version that produced it.
    QString getVersion() const;

    /// Return a string that represent the preferences used to generate
    /// the beats object.
    QString getSubVersion() const;

    void setSubVersion(const QString& subVersion);

    /// Initializes the BeatGrid to have a BPM of dBpm and the first beat offset
    /// of firstBeatFrame. Does not generate an updated() signal, since it is
    /// meant for initialization.
    void setGrid(Bpm dBpm, FramePos firstBeatFrame = kStartFramePos);

    /// Starting from frame, return the next beat
    /// in the track, or invalid beat if none exists. If frame refers to the location
    /// of a beat, the same beat is returned.
    Beat findNextBeat(FramePos frame) const;

    /// Starting from frame frame, return the previous
    /// beat in the track, or invalid beat if none exists. If frame refers to the
    /// location of beat, the same beat is returned.
    Beat findPrevBeat(FramePos frame) const;

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
    /// then the same beat is returned. If no beat can be found, returns kInvalidBeat.
    Beat findNthBeat(FramePos frame, int offset) const;

    int numBeatsInRange(FramePos startFrame, FramePos endFrame) const;

    /// Find the frame N beats away from frame. The number of beats may be
    /// negative and does not need to be an integer.
    FramePos findNBeatsFromFrame(FramePos fromFrame, double beats) const;

    /// Return an iterator to a container of Beats containing the Beats
    /// between startFrameNum and endFrameNum. THe BeatIterator must be iterated
    /// while a strong reference to the Beats object to ensure that the Beats
    /// object is not deleted. Caller takes ownership of the returned BeatsIterator
    std::unique_ptr<Beats::iterator> findBeats(FramePos startFrame,
            FramePos stopFrame) const;

    /**
     * Return Beat at (0 based) index
     * @param index
     * @return Beat object
     */
    Beat getBeatAtIndex(int index) const;

    /// Return the average BPM over the entire track if the BPM is
    /// valid, otherwise returns Bpm().
    Bpm getGlobalBpm() const;

    /// Return the average BPM over the range of n*2 beats centered around
    /// curFrameNum.  (An n of 4 results in an averaging of 8 beats).  Invalid
    /// BPM returns Bpm().
    Bpm getBpmAroundPosition(FramePos curFrame, int n) const;

    /// Sets the track signature starting at specified bar
    void setSignature(TimeSignature sig, int downbeatIndex);

    /// Translate all beats in the song by numFrames. Beats that lie
    /// before the start of the track or after the end of the track are not
    /// removed.
    void translate(FrameDiff_t numFrames);

    /// Scale the position of every beat in the song by dScalePercentage.
    void scale(enum BeatsInternal::BPMScale scale);

    /// Set bpm marker at a beat
    void setBpm(Bpm bpm, int beatIndex = 0);

    /// Returns the number of beats
    int size() const;

    /// Returns the frame number for the first beat, -1 is no beats
    FramePos getFirstBeatPosition() const;

    /// Returns the frame number for the last beat, -1 if no beats
    FramePos getLastBeatPosition() const;

    /**
     * Convert a non-downbeat to a downbeat shifting all downbeats
     * @param beatIndex Index of the beat to be converted to downbeat
     */
    void setAsDownbeat(int beatIndex);

    /// Prints debugging information in stderr
    friend QDebug operator<<(QDebug dbg, const BeatsPointer& arg);

    /// Update stream info encapsulating sample rate and track duration.
    void updateStreamInfo(const mixxx::audio::StreamInfo& streamInfo);

    /// Get the internal copyable Beats object.
    BeatsInternal getInternal() const;

    friend class ::Track;

  private:
    mutable QMutex m_mutex;
    BeatsInternal m_beatsInternal;

  signals:
    void updated();
};
} // namespace mixxx
