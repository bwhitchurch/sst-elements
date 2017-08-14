// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


class ShmemSendEntryBase: public SendEntryBase {
  public:
    ShmemSendEntryBase( int local_vNic ) : SendEntryBase( local_vNic ) { }
    ~ShmemSendEntryBase() { }
    
    MsgHdr::Op getOp() { return MsgHdr::Shmem; }
    void* hdr() { return &m_hdr; }
    size_t hdrSize() { return sizeof(m_hdr); }
  protected:
    Nic::ShmemMsgHdr m_hdr;
};

class ShmemCmdSendEntry: public ShmemSendEntryBase {
  public:
    ShmemCmdSendEntry( int local_vNic, NicShmemSendCmdEvent* event) : 
        ShmemSendEntryBase( local_vNic ), m_event( event ) { }
    int dst_vNic() { return m_event->getVnic(); }
    int dest() { return m_event->getNode(); }
  protected:
    NicShmemSendCmdEvent* m_event;
};

class ShmemRespSendEntry: public ShmemCmdSendEntry {
  public:
    ShmemRespSendEntry( int local_vNic, NicShmemSendCmdEvent* event ) : 
        ShmemCmdSendEntry( local_vNic, event )
    {
        assert( sizeof( m_hdr.respKey) == sizeof(m_event )); 
        m_hdr.vaddr = m_event->getFarAddr();
        m_hdr.length = m_event->getLength(); 
        m_hdr.respKey = (size_t) this;
    }
    bool shouldDelete() { return false; }

    size_t totalBytes() { return 0; } 
    bool isDone() { return true; }
    virtual void copyOut( Output&, int vc, int numBytes, 
            FireflyNetworkEvent&, std::vector<DmaVec>& ) {};
    NicShmemSendCmdEvent* getCmd() { return m_event; }
};

class ShmemGetvSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void(Hermes::Value&)> Callback;

    ShmemGetvSendEntry( int local_vNic, NicShmemSendCmdEvent* event, Callback callback  ) :
        ShmemRespSendEntry( local_vNic, event ), m_callback(callback)
    { 
        m_hdr.op = ShmemMsgHdr::Get; 
    }
    void callback( Hermes::Value& value ) { m_callback(value); }
  private:
    Callback  m_callback;
};

class ShmemFaddSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void(Hermes::Value&)> Callback;

    ShmemFaddSendEntry( int local_vNic, NicShmemSendCmdEvent* event, Callback callback  ) :
        ShmemRespSendEntry( local_vNic, event ), m_callback(callback)
    { 
        m_shmemMove = new ShmemSendMoveMem( event->getBacking(), event->getLength() );
        m_hdr.op = ShmemMsgHdr::Fadd; 
        m_hdr.dataType = m_event->getDataType();
    }
    ~ShmemFaddSendEntry() { delete m_shmemMove; }

    void callback( Hermes::Value& value ) { m_callback(value); }

    void copyOut( Output& dbg, int vc, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<DmaVec>&  vec) { 
        m_shmemMove->copyOut( dbg, vc, numBytes, ev, vec ); 
    }
  private:
    Callback  m_callback;
    ShmemSendMove* m_shmemMove;
};

class ShmemSwapSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void(Hermes::Value&)> Callback;
    ShmemSwapSendEntry( int local_vNic, NicShmemSwapCmdEvent* event, Callback callback  ) :
        ShmemRespSendEntry( local_vNic, event ), m_callback(callback)
    {
        m_shmemMove = new ShmemSendMoveValue( event->data );
        m_hdr.op = ShmemMsgHdr::Swap; 
        m_hdr.dataType = m_event->getDataType();
    }
    ~ShmemSwapSendEntry() { delete m_shmemMove; }

    void callback( Hermes::Value& value ) { m_callback(value); }

    void copyOut( Output& dbg, int vc, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<DmaVec>&  vec) { 
        m_shmemMove->copyOut( dbg, vc, numBytes, ev, vec ); 
    }
  private:
    Callback        m_callback;
    ShmemSendMove*  m_shmemMove;
};

class ShmemCswapSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void(Hermes::Value&)> Callback;
    ShmemCswapSendEntry( int local_vNic, NicShmemCswapCmdEvent* event, Callback callback  ) :
        ShmemRespSendEntry( local_vNic, event ), m_callback(callback)
    {
        m_shmemMove = new ShmemSendMove2Value( event->data, event->cond );
        m_hdr.op = ShmemMsgHdr::Cswap; 
        m_hdr.dataType = m_event->getDataType();
    }
    ~ShmemCswapSendEntry() { delete m_shmemMove; }

    void callback( Hermes::Value& value ) { m_callback(value); }

    void copyOut( Output& dbg, int vc, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<DmaVec>&  vec) { 
        m_shmemMove->copyOut( dbg, vc, numBytes, ev, vec ); 
    }
  private:
    Callback        m_callback;
    ShmemSendMove*  m_shmemMove;
};

class ShmemGetbSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void()> Callback;

    ShmemGetbSendEntry( int local_vNic, NicShmemSendCmdEvent* event, Callback callback  ) : 
        ShmemRespSendEntry( local_vNic, event ), m_callback(callback) 
    { 
        m_hdr.op = ShmemMsgHdr::Get; 
    }
    void callback() { m_callback(); }
  private:
    Callback  m_callback;
};

class ShmemPutSendEntry: public ShmemCmdSendEntry  {
  public:
    typedef std::function<void()> Callback;
    ShmemPutSendEntry( int local_vNic, NicShmemSendCmdEvent* event,
                                                Callback callback ) : 
        ShmemCmdSendEntry( local_vNic, event ),
        m_callback(callback)
    {
        m_hdr.op = ShmemMsgHdr::Put; 
        m_hdr.vaddr = m_event->getFarAddr();
        m_hdr.length = m_event->getLength(); 
        m_hdr.respKey = 0;
    }

    ~ShmemPutSendEntry() {
        m_callback( );
        delete m_event;
        delete m_shmemMove;
    }

    size_t totalBytes() { return m_hdr.length; } 
    bool isDone() { return m_shmemMove->isDone(); }
    void copyOut( Output& dbg, int vc, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<DmaVec>& vec ) {
        m_shmemMove->copyOut( dbg, vc, numBytes, ev, vec ); 
    };

  protected:
    Callback m_callback;
    ShmemSendMove* m_shmemMove;
};

class ShmemPutbSendEntry: public ShmemPutSendEntry  {
  public:
    ShmemPutbSendEntry( int local_vNic, NicShmemSendCmdEvent* event, void* backing,
                                                Callback callback ) : 
        ShmemPutSendEntry( local_vNic, event, callback )
    {
        m_shmemMove = new ShmemSendMoveMem( backing, event->getLength() );
    }
    ~ShmemPutbSendEntry() {
    }
};

class ShmemPutvSendEntry: public ShmemPutSendEntry  {
  public:
    ShmemPutvSendEntry( int local_vNic, NicShmemSendCmdEvent* event,
                                                Callback callback ) : 
        ShmemPutSendEntry( local_vNic, event, callback )
    {
        m_shmemMove = new ShmemSendMoveMem( event->getBacking(), event->getLength() );
    }
};

class ShmemPut2SendEntry: public ShmemSendEntryBase  {
  public:
    ShmemPut2SendEntry( int local_vNic, int destNode, int dest_vNic,
            void* ptr, size_t length, uint64_t key ) :
        ShmemSendEntryBase( local_vNic ),
        m_node( destNode ),
        m_vnic(dest_vNic),
        m_value(NULL)
    {
        init( length, key );
        m_shmemMove = new ShmemSendMoveMem( ptr, length );
    }
    ShmemPut2SendEntry( int local_vNic, int destNode, int dest_vNic,
            Hermes::Value* value, uint64_t key ) :
        ShmemSendEntryBase( local_vNic ),
        m_node( destNode ),
        m_vnic(dest_vNic),
        m_value(value) 
    {
        init( value->getLength(), key );
        m_shmemMove = new ShmemSendMoveValue( *value );
    }

    void init( uint64_t length, uint64_t key ) {
        m_hdr.op = ShmemMsgHdr::Put; 
        m_hdr.respKey = key;
        m_hdr.length = length; 
    }

    ~ShmemPut2SendEntry() {
        delete m_shmemMove;
        if ( m_value) { delete m_value; }
    }
    int dst_vNic() { return m_vnic; }
    int dest() { return m_node; }

    size_t totalBytes() { return m_hdr.length; } 
    bool isDone() { return m_shmemMove->isDone(); }
    void copyOut( Output& dbg, int vc, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<DmaVec>& vec ) {
        m_shmemMove->copyOut( dbg, vc, numBytes, ev, vec ); 
    };

  private:
    ShmemSendMove* m_shmemMove;
    int m_vnic;
    int m_node;
    Hermes::Value* m_value;
};
