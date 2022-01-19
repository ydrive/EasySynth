// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class ISequencer;
class ULevelSequence;
class UMovieScene;
class UMovieSceneCameraCutSection;
class UMovieSceneTrack;


/**
 * Class that loads the sequencer for the given level sequence asset,
 * checks the validity of all needed components and provides access to them.
 *
 * It should be used locally during an action that requires the sequencer,
 * as it only holds weak references to its contents.
*/
class FSequencerWrapper
{
public:
	/** Opens the requested sequence */
	bool OpenSequence(ULevelSequence* LevelSequence);

	/** Access the movie scene */
	UMovieScene* GetMovieScene()
	{
		check(MovieScene)
		return MovieScene;
	}

	/** Access the camera cut track */
	UMovieSceneTrack* GetCameraCutTrack()
	{
		check(CameraCutTrack)
		return CameraCutTrack;
	}

	/** Access the movie scene cut sections */
	TArray<UMovieSceneCameraCutSection*>& GetMovieSceneCutSections();

	/** Access the sequencer */
	ISequencer* GetSequencer()
	{
		check(WeakSequencer.IsValid())
		return WeakSequencer.Pin().Get();
	}

private:
	/** Sequence movie scene */
	UMovieScene* MovieScene;

	/** Sequence camera cut track */
	UMovieSceneTrack* CameraCutTrack;

	/** Camera track cut sections */
	TArray<UMovieSceneCameraCutSection*> MovieSceneCutSections;

	/** Level sequence editor */
	TWeakPtr<ISequencer> WeakSequencer;
};
