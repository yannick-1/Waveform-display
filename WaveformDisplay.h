
#pragma once

class MainContentComponent:
    public AudioAppComponent,
    private ChangeListener,
    private Timer
{
    public:
        MainContentComponent():
            state(Stopped),
            // Cache objects must be constructed with the number of thumbnails to store.
            thumbnailCache(5),                            
            thumbnail(512, formatManager, thumbnailCache)
    
        {
            addAndMakeVisible(&openButton);
            openButton.setButtonText("Open...");
            openButton.onClick = [this] { openButtonClicked(); };
            /* Grey colour */
            openButton.setColour(TextButton::buttonColourId, Colour(97, 95, 107));

            addAndMakeVisible(&playButton);
            playButton.setButtonText("Play");
            playButton.onClick = [this] { playButtonClicked(); };
            /* Green colour */
            playButton.setColour(TextButton::buttonColourId, Colour(100, 194, 172));
            playButton.setEnabled(false);

            addAndMakeVisible(&stopButton);
            stopButton.setButtonText("Stop");
            stopButton.onClick = [this] { stopButtonClicked(); };
            /* Red colour */
            stopButton.setColour(TextButton::buttonColourId, Colour(253, 126, 128));
            stopButton.setEnabled(false);

            setSize(600, 400);

            formatManager.registerBasicFormats();
            transportSource.addChangeListener(this);
            thumbnail.addChangeListener(this);            

            setAudioChannels(2, 2);
            startTimer(16);
        }

        ~MainContentComponent()
        {
            shutdownAudio();
        }

        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
        {
            transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
        }

        void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override
        {
            if (readerSource.get() == nullptr)
                bufferToFill.clearActiveBufferRegion();
            else
                transportSource.getNextAudioBlock(bufferToFill);
        }

        void releaseResources() override
        {
            transportSource.releaseResources();
        }

        void paint(Graphics& g) override
        {
            Rectangle<int> thumbnailBounds(10, 100, getWidth() - 20, getHeight() - 120);
            /* Background colour */
            g.fillAll(Colour(39, 47, 57));
            
            // Check to see if file is loaded. 
            if (thumbnail.getNumChannels() == 0)
                paintIfNoFileLoaded(g, thumbnailBounds);
            else
                paintIfFileLoaded(g, thumbnailBounds);
        }

        void resized() override
        {
            openButton.setBounds(10, 10, getWidth() - 20, 20);
            playButton.setBounds(10, 40, getWidth() - 20, 20);
            stopButton.setBounds(10, 70, getWidth() - 20, 20);
        }

        // Determines if the change is being broadcasted from the AudioTransportSource object or the AudioThumbnail object.
        void changeListenerCallback(ChangeBroadcaster* source) override
        {
            if (source == &transportSource) 
            {
                transportSourceChanged();
            }
            if (source == &thumbnail)
            {
                thumbnailChanged();
            }
        }

    private:
        enum TransportState
        {
            Stopped,
            Starting,
            Playing,
            Stopping
        };

        void changeState(TransportState newState)
        {
            if (state != newState)
            {
                state = newState;

                switch (state)
                {
                    case Stopped:
                        stopButton.setEnabled(false);
                        playButton.setEnabled(true);
                        transportSource.setPosition(0.0);
                        break;

                    case Starting:
                        playButton.setEnabled(false);
                        transportSource.start();
                        break;

                    case Playing:
                        stopButton.setEnabled(true);
                        break;

                    case Stopping:
                        transportSource.stop();
                        break;

                    default:
                        jassertfalse;
                        break;
                }
            }
        }

        // Responds to changes in the AudioTransportSource object.
        void transportSourceChanged()
        {
            changeState(transportSource.isPlaying() ? Playing : Stopped);
        }

        void thumbnailChanged()
        {
            repaint();
        }

        // If we have no file loaded then display the message "No File Loaded".
        void paintIfNoFileLoaded(Graphics& g, const Rectangle<int>& thumbnailBounds)
        {
            /* Grey colour */
            g.setColour(Colour(54, 60, 69));
            g.fillRect(thumbnailBounds);
            g.setColour(Colours::white);      
            g.drawFittedText("No file loaded", thumbnailBounds, Justification::centred, 1.0f);
        }

        // If file is loaded, draw the waveform. 
        void paintIfFileLoaded(Graphics& g, const Rectangle<int>& thumbnailBounds)
        {
            /* Player background colour */
            g.setColour(Colour(54, 60, 69));
            g.fillRect(thumbnailBounds);
            /* Waveform colour */
            g.setColour(Colour(250, 196, 47));

            auto audioLength(thumbnail.getTotalLength());
            thumbnail.drawChannels(g,                                      
                                    thumbnailBounds,
                                    0.0,                                    // start time
                                    audioLength,                            // end time
                                    1.0f);                                  // vertical zoom

            auto audioPosition(transportSource.getCurrentPosition());
            // The position is calculated as a proportion of the total length of the audio file. 
            // The position to draw the line is  on the same proportion 
            // of the width of the rectangle that the thumbnail is drawn within. 
            auto drawPosition((audioPosition / audioLength) * thumbnailBounds.getWidth()
                               + thumbnailBounds.getX());
            /* Teal colour */
            g.setColour(Colour( 156, 209, 201));
            // Draws a line that is 2 pixels wide between the top and bottom of the rectangle.                                
            g.drawLine(drawPosition, thumbnailBounds.getY(), drawPosition, thumbnailBounds.getBottom(), 2.0f);

        }

        void openButtonClicked()
        {
            FileChooser chooser("Select a Wave file to play...",
                                {},
                                "*.wav");

            if (chooser.browseForFileToOpen())
            {
                File file(chooser.getResult());
                auto* reader = formatManager.createReaderFor(file);

                if(reader != nullptr)
                {
                    std::unique_ptr<AudioFormatReaderSource> newSource(new AudioFormatReaderSource(reader, true));
                    transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                    playButton.setEnabled(true);
                    thumbnail.setSource(new FileInputSource(file));        
                    readerSource.reset(newSource.release());
                }
            }
        }

        void playButtonClicked()
        {
            changeState(Starting);
        }

        void stopButtonClicked()
        {
            changeState(Stopping);
        }

        void timerCallback() override
        {
            repaint();
        }

        TextButton openButton;
        TextButton playButton;
        TextButton stopButton;

        AudioFormatManager formatManager;                    
        std::unique_ptr<AudioFormatReaderSource> readerSource;
        AudioTransportSource transportSource;
        TransportState state;
        // Caches low resolution version of audio files.
        AudioThumbnailCache thumbnailCache;
        AudioThumbnail thumbnail;                            

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
    };
