
#ifndef _MIDIINOUT_H
#define _MIDIINOUT_H


namespace base {


struct MidiEvent {
	union{
		struct {
			BYTE	msg;
			BYTE	data1;
			BYTE	data2;
			BYTE	unused;
		};
		DWORD	midi_raw;
	};
	DWORD	timestamp;
};


class MidiInput {
public:
	enum { BUFFER_SIZE = 65536 };

	MidiInput();
	~MidiInput();

	bool		Connect(int device_id);
	bool		IsConnected()	{ return connected; }
	MidiEvent	*GetMsg()		{ return (!connected || get_pos == put_pos) ? NULL : buffer + get_pos; }
	void		NextMsg()		{ get_pos = (get_pos+1) & (BUFFER_SIZE-1); }
	bool		IsMessage()		{ return (connected && get_pos != put_pos); }

	void		SetTimestamp(DWORD _timestamp)	{ timestamp = _timestamp; }


	static int			GetDeviceCount()				{ return midiInGetNumDevs(); }
	static const char	*GetDeviceName(int device_id);


private:
	friend void CALLBACK _midi_in_proc(HMIDIIN,UINT,DWORD_PTR,DWORD_PTR,DWORD_PTR);

	HMIDIIN		midi_in;
	MidiEvent	buffer[BUFFER_SIZE];
	int			get_pos;
	int			put_pos;
	DWORD		timestamp;
	bool		connected;

	MidiInput(const MidiInput &mi);		// does not exist

};

class MidiOutput {
public:

	MidiOutput();
	~MidiOutput();

	bool Connect(int device_id);
	bool IsConnected()		{ return connected; }
	void SendMsg(DWORD msg)	{ midiOutShortMsg(midi_out,msg); }

	static int			GetDeviceCount()				{ return midiOutGetNumDevs(); }
	static const char	*GetDeviceName(int device_id);


private:
	HMIDIOUT	midi_out;
	bool		connected;

	MidiOutput(const MidiOutput &mi);		// does not exist

};


}


#endif
