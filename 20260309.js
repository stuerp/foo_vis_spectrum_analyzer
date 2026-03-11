class AudioProvider extends AudioWorkletProcessor
{
    constructor()
    {
        super();

        this.dataArrays           = [];
        this.bufferSize           = 32768; // can handle more than 32768 samples of PCM data unlike in AnalyserNode.getFloatTimeDomainData, which is capped at 32768 samples
        this.bufferIdx            = 0;
        this.currentTimeInSamples = 0;

        this.port.onmessage = (e) =>
        {
            const
                AudioChunks = [],
                retrievalWindowSize = e.data ? Math.min(this.bufferSize, currentFrame - this.currentTimeInSamples) : this.bufferSize,
                TimeOffset = this.bufferSize - retrievalWindowSize;

            for (let ChannelIndex = 0; ChannelIndex < this.dataArrays.length; ++ChannelIndex)
            {
                AudioChunks[ChannelIndex] = [];

                for (let i = 0; i < this.dataArrays[ChannelIndex].length - TimeOffset; ++i)
                {
                    const Data = this.dataArrays[ChannelIndex][((this.bufferIdx + i + TimeOffset) % this.bufferSize + this.bufferSize) % this.bufferSize];

                    AudioChunks[ChannelIndex][i] = (Data !== undefined) ? Data : 0;
                }
            }

            this.port.postMessage({ audioChunks: AudioChunks });
            this.currentTimeInSamples = currentFrame;
        };
    }
  
    process(inputs, _, _2)
    {
        if (inputs[0].length <= 0)
            return true;

        this.dataArrays.length = inputs[0].length;

        for (let i = 0; i < this.dataArrays.length; i++)
        {
            if (this.dataArrays[i] === undefined)
                this.dataArrays[i] = new Array(this.bufferSize);
            else
            {
                this.dataArrays[i].length = this.bufferSize;
            }
        }
    
        for (let i = 0; i < inputs[0][0].length; i++)
        {
            this.bufferIdx = Math.min(this.bufferIdx, this.bufferSize-1);

            for (let ChannelIndex = 0; ChannelIndex < inputs[0].length; ChannelIndex++)
            {
                this.dataArrays[ChannelIndex][this.bufferIdx] = inputs[0][ChannelIndex][i];
            }

            this.bufferIdx = ((this.bufferIdx + 1) % this.bufferSize + this.bufferSize) % this.bufferSize;
        }

        return true;
    }
}

registerProcessor('audio-provider', AudioProvider);

function map(x, min, max, targetMin, targetMax)
{
    return (x - min) / (max - min) * (targetMax - targetMin) + targetMin;
}
  
function clamp(x, min, max)
{
    return Math.min(Math.max(x, min), max);
}
  
function idxWrapOver(x, length)
{
    return (x % length + length) % length;
}

// necessary parts for audio context and audio elements respectively

const _AudioContext = new AudioContext();

const audioPlayer = document.getElementById('audio');
const localAudioElement = document.getElementById('audioFileInput');

localAudioElement.addEventListener('change', loadLocalFile);

// canvas is for displaying visuals

const canvas = document.getElementById('canvas'),

ctx = canvas.getContext('2d'),

container = document.getElementById('container');

const _AudioSource = _AudioContext.createMediaElementSource(audioPlayer);
const _Analyser = _AudioContext.createAnalyser();

_Analyser.fftSize = 32768; // maxes out FFT size

// variables
const _Frames = [];

let _SampleIndex = 0;

const _DelayNode = _AudioContext.createDelay();

_AudioSource.connect(_DelayNode);
_DelayNode.connect(_AudioContext.destination);

//_AudioSource.connect(_AudioContext.destination);
_AudioSource.connect(_Analyser);

let
    _AudioProvider,
    _SampleRate = _AudioContext.sampleRate;

const kWeighted = [];
const
    customDSPSource = document.getElementById('AudioProvider'),
    dspSourceBlob = new Blob([customDSPSource.innerText], { type: 'application/javascript' }),
    dspSourceUrl = URL.createObjectURL(dspSourceBlob);

const _Settings =
{
    resetAverages: resetSmoothedValues,
    mode: 'float32',
    drawMode: 'lumi',
    intBitDepth: 24,
    clipIntSamples: true,
    useTrueSymmetry: true,
    useLittleEndian: false,
    useLogScale: false,
    gamma: 1,
    freeze: false,
    darkMode: false,
    useGradient: true,
    autoReset: true,
    resetBoth: resetBoth,
    FrameCount: 1920,
    compensateDelay: true
},

averagingDomains =
{
    'Linear': 'linear',
    'Squared (RMS)': 'rms',
    'Logarithmic': 'log'
},

loader =
{
    url: '',

    load: function ()
    {
        audioPlayer.src = this.url;
        audioPlayer.play();
    },

    loadLocal: function ()
    {
        localAudioElement.click();
    },

    toggleFullscreen: _ =>
    {
        if (document.fullscreenElement === canvas)
            document.exitFullscreen();else

        canvas.requestFullscreen();
    }
};

// dat.GUI for quick customization
let gui = new dat.GUI();

    gui.add(loader, 'url').name('URL');
    gui.add(loader, 'load').name('Load');
    gui.add(loader, 'loadLocal').name('Load from local device');

    let settings = gui.addFolder('Visualization settings');

        settings.add(_Settings, 'fftSize', 32, 65536, 1).name('Buffer length').onChange(resetBoth);

        const amplitudeFolder = settings.addFolder('Amplitude');

            amplitudeFolder.add(_Settings, 'useLogScale').name('Use logarithmic amplitude scale');
            amplitudeFolder.add(_Settings, 'gamma', 0.1, 10).name('Gamma');

        const bitmeterFolder = settings.addFolder('Bitmeter settings');

            bitmeterFolder.add(_Settings, 'mode',
            {
                '32-bit floating point': 'float32',
                '64-bit floating point': 'float64',
                'N-bit integer': 'int'
            }).name('Bitmeter mode');

                bitmeterFolder.add(_Settings, 'intBitDepth', 2, 32, 1).name('Integer bit depth');
                bitmeterFolder.add(_Settings, 'clipIntSamples').name('Clip integer samples instead of allowing wrap-around/wrap-overs on integer formats');
                bitmeterFolder.add(_Settings, 'useTrueSymmetry').name('Use true symmetry for conversion to integer');
                bitmeterFolder.add(_Settings, 'useLittleEndian').name('Use little endian instead of big endian');

            settings.add(_Settings, 'drawMode',
            {
                'Bars': 'bars',
                'Luminance': 'lumi'
            }).name('Draw mode');

                settings.add(_Settings, 'freeze').name('Freeze analyzer');
                settings.add(_Settings, 'darkMode').name('Dark mode');
                settings.add(_Settings, 'useGradient').name('Use brighter color on dark mode');
                settings.add(_Settings, 'autoReset').name('Enable auto-reset');
                settings.add(_Settings, 'compensateDelay').name('Compensate for delay');
                settings.add(_Settings, 'resetBoth').name('Reset visualizer');

    gui.add(loader, 'toggleFullscreen').name('Toggle fullscreen mode');

function resetBoth()
{
    resetSmoothedValues();
}

function resetSmoothedValues()
{
    _Frames.length = 0;
    _SampleIndex = 0;
}

function resizeCanvas()
{
    const scale = devicePixelRatio,
        isFullscreen = document.fullscreenElement === canvas;
        canvas.width  = (isFullscreen ? innerWidth  : container.clientWidth)  * scale;
        canvas.height = (isFullscreen ? innerHeight : container.clientHeight) * scale;
}

addEventListener('click', () =>
{
    if (_AudioContext.state == 'suspended')
        _AudioContext.resume();
});

addEventListener('resize', resizeCanvas);

resizeCanvas();

function autoReset()
{
    if (_Settings.autoReset && !_Settings.freeze)
        resetBoth();
}

// this below makes it more faithful to how foobar2000 visualizations work
audioPlayer.addEventListener('play', autoReset);
audioPlayer.addEventListener('seeked', autoReset);

function loadLocalFile(event)
{
    const
        file = event.target.files[0],

        reader = new FileReader();

        reader.onload = e =>
        {
            audioPlayer.src = e.target.result;
            audioPlayer.play();
        };

        reader.readAsDataURL(file);
}

_AudioContext.audioWorklet.addModule(dspSourceUrl).then(() =>
{
    _AudioProvider = new AudioWorkletNode(_AudioContext, 'audio-provider');

    _AudioSource.connect(_AudioProvider);

    _AudioProvider.port.postMessage(0);

    _AudioProvider.port.onmessage = e =>
    {
        if (!_Settings.freeze)
            analyzeChunk(e.data.audioChunks);

        _AudioProvider.port.postMessage(1);
    };

    _AudioProvider.onprocessorerror = e =>
    {
        console.log(e.message);
    };

    visualize();
})
.catch(e =>
{
    console.log(e.message);
});

function analyzeChunk(audioChunks)
{
    const dataset = [];

    let ChunkSize = 0;

    for (const x of audioChunks)
    {
        ChunkSize = Math.max(ChunkSize, x.length);
    }

    _Frames.length = audioChunks.length;

    for (let i = 0; i < audioChunks.length; i++)
    {
        const length = _Settings.FrameCount;

        if (_Frames[i] === undefined)
            _Frames[i] = new Array(length);
        else
        if (_Frames[i].length !== length)
            _Frames[i].length = length;
    }

    for (let i = 0; i < ChunkSize; ++i)
    {
        for (let ChannelIndex = 0; ChannelIndex < audioChunks.length; ++ChannelIndex)
        {
            _Frames[ChannelIndex][_SampleIndex] = audioChunks[ChannelIndex][i];
        }

        _SampleIndex = idxWrapOver(_SampleIndex + 1, _Settings.FrameCount);
    }

    _SampleRate = _AudioContext.sampleRate;
}

function visualize()
{
    _DelayNode.delayTime.value = _Settings.FrameCount / _AudioContext.sampleRate * _Settings.compensateDelay;

    const BitmeterData = [];

    for (let ChannelIndex = 0; ChannelIndex < _Frames.length; ++ChannelIndex)
    {
        BitmeterData[ChannelIndex] = [];

        for (let i = 0; i < _Frames[ChannelIndex].length; ++i)
        {
            const
                x = _Frames[ChannelIndex][i],

                y = 2 ** _Settings.intBitDepth / 2,
                z = _Settings.clipIntSamples ? clamp(x, -1, _Settings.useTrueSymmetry ? (y - 1) / y : 1) : x,
                w = _Settings.useTrueSymmetry ? z * y : map(z, -1, 1, -y, y - 1),

            arrayOfBits = (_Settings.mode === 'float32') || (_Settings.mode === 'float64') ?
                FloatToBitArray(x, (_Settings.mode === 'float32') ? 32 : 64, _Settings.useLittleEndian) :
                FixedToBitArray(w, _Settings.intBitDepth,                   !_Settings.useLittleEndian);

            updateAccumulatedData(BitmeterData[ChannelIndex], arrayOfBits);
        }
    }

    const
        fgColor = _Settings.darkMode ? _Settings.useGradient ? '#c0c0c0' : '#fff' : '#000',
        bgColor = _Settings.darkMode ? _Settings.useGradient ? '#202020' : '#000' : '#fff';

    ctx.globalCompositeOperation = 'source-over';

    // Draw the background.
    ctx.fillStyle = bgColor;
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // Draw the bars.
    ctx.fillStyle   = fgColor;
    ctx.strokeStyle = fgColor;

    for (let ChannelIndex = 0; ChannelIndex < BitmeterData.length; ++ChannelIndex)
    {
        ctx.globalAlpha = 1;

        BitmeterData[ChannelIndex].map((x, i, arr) =>
        {
            const ScaledBitmeterValue = ScaleBitmeterValue(x, _Settings.FrameCount, _Settings.gamma, _Settings.useLogScale);

            ctx.globalAlpha = (_Settings.drawMode === 'lumi') ? ScaledBitmeterValue : ctx.globalAlpha;
            ctx.fillRect
            (
                i * canvas.width / arr.length + 1,
                (ChannelIndex + 1) * (canvas.height / BitmeterData.length),
                Math.max(canvas.width / arr.length - 2, 1),
                map((_Settings.drawMode === 'bars') ? ScaledBitmeterValue : 1, 0, 1, 0, -canvas.height / BitmeterData.length)
            );
        });

        ctx.globalAlpha = 0.5;

        if (ChannelIndex < BitmeterData.length - 1)
        {
            ctx.beginPath();

            ctx.lineTo(0,            (ChannelIndex + 1) * canvas.height / BitmeterData.length);
            ctx.lineTo(canvas.width, (ChannelIndex + 1) * canvas.height / BitmeterData.length);

            ctx.stroke();
        }
    }

    ctx.globalAlpha = 1;

    requestAnimationFrame(visualize);
}

function ScaleBitmeterValue(x, frameCount, gamma = 1, useLogScale = false)
{
    if (useLogScale)
    {
        return map(Math.log2(x), -1, Math.log2(frameCount), 0, 1);
    }
    else
    {
        return (x / frameCount) ** (1 / gamma);
    }
}

// Converts a number to an array of bits representing floating-point (either 32-bit or 64-bit)
function FloatToBitArray(num, bits = 32, littleEndian = false)
{
    let buffer = new ArrayBuffer(bits / 8);
    let view = new DataView(buffer);

    // Write the number to the buffer as a float
    if (bits === 32)
    {
        view.setFloat32(0, num, littleEndian); // false for big-endian
    }
    else
    {
        view.setFloat64(0, num, littleEndian);
    }

    let bitArray = [];

    // Read bytes and extract bits
    for (let i = 0; i < bits / 8; ++i)
    {
        let byte = view.getUint8(i);

        for (let j = 7; j >= 0; --j)
        {
            bitArray.push(byte >> j & 1);
        }
    }

    return bitArray;
}

// Converts to N-bit integer representation.
function FixedToBitArray(num, bitCount, isBigEndian = true)
{
    const mask = (bitCount === 32) ? 0xFFFFFFFF : (1 << bitCount) - 1;

    let Value = (num & mask) >>> 0;

    let bitArray = [];

    const ByteCount = Math.ceil(bitCount / 8);

    for (let i = 0; i < ByteCount; ++i)
    {
        const ByteShift = isBigEndian ? (ByteCount - 1 - i) * 8 : i * 8;

        const byte = Value >> ByteShift & 0xFF;

        for (let j = 7; j >= 0; j--)
        {
            bitArray.push(byte >> j & 1);
        }
    }

    return isBigEndian ? bitArray.slice(-bitCount) : bitArray.slice(0, bitCount);
}

function updateAccumulatedData(target, arrayOfBits)
{
    target.length = arrayOfBits.length;

    for (let i = 0; i < arrayOfBits.length; ++i)
    {
        const
            prevResult = target[i],

            x = arrayOfBits[i];

            target[i] = (isFinite(prevResult) ? prevResult : 0) + (isFinite(x) ? x : 0);
    }
}
