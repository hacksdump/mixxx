// Created on: 30/nov/2011
// Author: vittorio

#ifndef BEATUTILS_H_
#define BEATUTILS_H_

// to tell the msvs compiler about `isnan`
#include "util/math.h"

#include <QVector>

// Included to get mixxx::kEngineChannelCount
#include "engine/engine.h"
#include "track/bpm.h"

class BeatUtils {
  public:
    static void printBeatStatistics(const QVector<double>& beats, int SampleRate);

    static double constrainBpm(double bpm, int min_bpm, int max_bpm, bool aboveRange) {
        if (bpm <= 0.0 || min_bpm < 0 || max_bpm < 0 ||
            min_bpm >= max_bpm ||
            (bpm >= min_bpm && bpm <= max_bpm)) {
            return bpm;
        }

        if (isnan(bpm) || isinf(bpm)) {
            return 0.0;
        }

        if (!aboveRange) {
            while (bpm > max_bpm) {
                bpm /= 2.0;
            }
        }
        while (bpm < min_bpm) {
            bpm *= 2.0;
        }

        return bpm;
    }

    /*
     * This method detects the BPM given a set of beat positions.
     * We compute the average local BPM of by considering 8 beats
     * at a time. Internally, a sorted list of average BPM values is constructed
     * from which the statistical median is computed. This value provides
     * a pretty good guess of the global BPM value.
     */
    static mixxx::Bpm calculateBpm(const QVector<double>& beats,
            int sampleRate,
            int min_bpm,
            int max_bpm);
    static double findFirstCorrectBeat(const QVector<double>& rawBeats,
            int SampleRate,
            double global_bpm);

    /* This implement a method to find the best offset so that
     * the grid generated from bpm is close enough to the one we get from vamp.
     */
    static double calculateOffset(const QVector<double>& beats1,
            double bpm1,
            const QVector<double>& beats2,
            int SampleRate);

    // By default Vamp does not assume a 4/4 signature. This is basically a good
    // property of Vamp, however, it leads to inaccurate beat grids if a 4/4
    // signature is given.  What is the problem? Almost all modern dance music
    // from the last decades refer to 4/4 signatures. Given a set of beat frame
    // positions, this method calculates the position of the first beat assuming
    // the beats have a fixed tempo given by globalBpm.
    static double calculateFixedTempoFirstBeat(
            bool enableOffsetCorrection,
            const QVector<double>& rawbeats,
            int sampleRate,
            int totalSamples,
            double globalBpm);

    static double samplesToFrames(double samples) {
        return samples / mixxx::kEngineChannelCount;
    }
    static double framesToSamples(double frames) {
        return frames * mixxx::kEngineChannelCount;
    }

  private:
    static double computeSampleMedian(QList<double> sortedItems);
    static double computeFilteredWeightedAverage(
            const QMap<double, int>& frequencyTable,
            double filterCenter,
            double filterTolerance,
            QMap<double, int>* filteredFrequencyTable);
    static QList<double> computeWindowedBpmsAndFrequencyHistogram(
            const QVector<double>& beats,
            int windowSize,
            int windowStep,
            int sampleRate,
            QMap<double, int>* frequencyHistogram);
};

#endif /* BEATUTILS_H_ */
