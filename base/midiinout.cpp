
#include "base.h"

namespace base {


static HMIDIOUT midi_thru;


static void CALLBACK _midi_in_proc(HMIDIIN hMidiIn,UINT wMsg,DWORD_PTR dwInstance,DWORD_PTR dwParam1,DWORD_PTR dwParam2)
{
	if(wMsg != MIM_DATA)
		return;

	MidiInput *input = (MidiInput*)dwInstance;
	DWORD nput = (input->put_pos + 1) & (MidiInput::BUFFER_SIZE - 1);
	if(nput == input->get_pos)
		return;

	MidiEvent *ev = input->buffer + input->put_pos;
	ev->midi_raw = dwParam1;
	ev->timestamp = dwParam2;

	if(midi_thru)
		midiOutShortMsg(midi_thru,dwParam1);

	input->put_pos = nput;
}



MidiInput::MidiInput()
{
	midi_in = NULL;
	get_pos = 0;
	put_pos = 0;
	timestamp = 0;
	connected = false;
}


MidiInput::~MidiInput()
{
	midiInClose(midi_in);
}


bool MidiInput::Connect(int device_id)
{
	midiInClose(midi_in);

	midi_in = NULL;
	connected = false;

	MMRESULT res = midiInOpen(&midi_in,device_id,(DWORD_PTR)_midi_in_proc,(DWORD_PTR)this,CALLBACK_FUNCTION);
	if(res != MMSYSERR_NOERROR)
		return false;

	res = midiInStart(midi_in);
	if(res != MMSYSERR_NOERROR)
		return false;

	connected = true;

	return true;
}


const char *MidiInput::GetDeviceName(int device_id)
{
	static MIDIINCAPS caps;
	MMRESULT res = midiInGetDevCaps((UINT)device_id,&caps,sizeof(caps));

	if(res != MMSYSERR_NOERROR)
		return "???";

	return caps.szPname;
}



MidiOutput::MidiOutput()
{
	midi_out = NULL;
	connected = false;
}


MidiOutput::~MidiOutput()
{
	midiOutClose(midi_out);
}


bool MidiOutput::Connect(int device_id)
{
	midiOutClose(midi_out);

	midi_out = NULL;
	connected = false;

	MMRESULT res = midiOutOpen(&midi_out,device_id,0,0,CALLBACK_NULL);
	if(res != MMSYSERR_NOERROR)
		return false;

//	midi_thru = output->midi_out;
	connected = true;

	return true;
}


const char *MidiOutput::GetDeviceName(int device_id)
{
	static MIDIOUTCAPS caps;
	MMRESULT res = midiOutGetDevCaps((UINT)device_id,&caps,sizeof(caps));

	if(res != MMSYSERR_NOERROR)
		return "???";

	return caps.szPname;
}

}
