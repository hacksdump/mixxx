#ifndef BEATFACTORY_H
#define BEATFACTORY_H

#include <QHash>

#include "track/beats.h"
#include "track/track.h"

class BeatFactory {
  public:
    static mixxx::BeatsPointer loadBeatsFromByteArray(const TrackPointer& track,
            QString beatsVersion,
            QString beatsSubVersion,
            const QByteArray& beatsSerialized);
    static mixxx::BeatsPointer makeBeatGrid(const TrackPointer& track,
            double dBpm,
            double dFirstBeatSample);

    static QString getPreferredVersion(const bool bEnableFixedTempoCorrection);

    static QString getPreferredSubVersion(
        const bool bEnableFixedTempoCorrection,
        const bool bEnableOffsetCorrection,
        const int iMinBpm, const int iMaxBpm,
        const QHash<QString, QString> extraVersionInfo);

    static mixxx::BeatsPointer makePreferredBeats(const TrackPointer& track,
            QVector<double> beats,
            const QHash<QString, QString> extraVersionInfo,
            const bool bEnableFixedTempoCorrection,
            const bool bEnableOffsetCorrection,
            const int iTotalSamples,
            const int iMinBpm,
            const int iMaxBpm);

    static void deleteBeats(mixxx::Beats* pBeats);

  private:
    
};

#endif /* BEATFACTORY_H */
