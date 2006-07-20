#ifndef __OBJECTER_H
#define __OBJECTER_H

#include "include/types.h"
#include "include/bufferlist.h"

#include <list>
#include <map>
#include <ext/hash_map>
using namespace std;
using namespace __gnu_cxx;

class Context;
class Messenger;
class OSDMap;
class Message;

class Objecter {
 public:  
  Messenger *messenger;
  OSDMap    *osdmap;
  
 private:
  tid_t last_tid;

  /*** track pending operations ***/
  // read
 public:
  class OSDRead {
  public:
	bufferlist *bl;
	list<ObjectExtent> extents;
	Context *onfinish;
	map<tid_t, ObjectExtent> ops;
	map<object_t, bufferlist*> read_data;  // bits of data as they come back

	OSDRead(bufferlist *b) : bl(b), onfinish(0) {}
  };

  // write
  class OSDWrite {
  public:
	list<ObjectExtent> extents;
	bufferlist bl;
	Context *onack;
	Context *oncommit;
	map<tid_t, ObjectExtent> waitfor_ack;
	map<tid_t, ObjectExtent> waitfor_commit;

	OSDWrite(bufferlist &b) : bl(b), onack(0), oncommit(0) {}
  };

  // zero
  class OSDZero {
  public:
	list<ObjectExtent> extents;
	Context *onack;
	Context *oncommit;
	map<tid_t, ObjectExtent> waitfor_ack;
	map<tid_t, ObjectExtent> waitfor_commit;

	OSDZero() : onack(0), oncommit(0) {}
  };

 private:
  // pending ops
  hash_map<tid_t,OSDRead*>  op_read;
  hash_map<tid_t,OSDWrite*> op_write;
  hash_map<tid_t,OSDZero*>  op_zero;

  hash_map<int, set<tid_t> > osd_tids;
  hash_map<tid_t, int>       tid_osd;


 public:
  Objecter(Messenger *m, OSDMap *om) : 
	messenger(m), osdmap(om),
	last_tid(0)
	{}
  ~Objecter() {
	// clean up op_*
	// ***
  }

  // messages
 public:
  void dispatch(Message *m);
  void handle_osd_op_reply(class MOSDOpReply *m);
  void handle_osd_read_reply(class MOSDOpReply *m);
  void handle_osd_write_reply(class MOSDOpReply *m);
  void handle_osd_zero_reply(MOSDOpReply *m);
  void handle_osd_map(class MOSDMap *m);

 private:

  void readx_submit(OSDRead *rd, ObjectExtent& ex);
  void writex_submit(OSDWrite *wr, ObjectExtent& ex);
  void kick_requests(set<int> &kick);


  // public interface
 public:
  bool is_active() {
	return !(op_read.empty() && op_write.empty());
  }

  int readx(OSDRead *read, Context *onfinish);
  int writex(OSDWrite *write, Context *onack, Context *oncommit);
  int zerox(OSDZero *zero, Context *onack, Context *oncommit);

  tid_t read(object_t oid, off_t off, size_t len, bufferlist *bl, 
			 Context *onfinish);
  tid_t write(object_t oid, off_t off, size_t len, bufferlist &bl, 
			  Context *onack, Context *oncommit);
  tid_t zero(object_t oid, off_t off, size_t len,  
			 Context *onack, Context *oncommit);
};

#endif
