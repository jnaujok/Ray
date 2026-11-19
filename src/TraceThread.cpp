int CTraceThread::GetScene( CStream &oStream )
{
    m_oScene.Serialize( oStream, TRUE );
}