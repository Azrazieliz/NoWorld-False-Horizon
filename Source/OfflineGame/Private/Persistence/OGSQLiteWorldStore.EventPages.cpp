#include "Persistence/OGSQLiteWorldStore.h"
#include "sqlite/sqlite3.h"

namespace
{
bool EventPageBindText(sqlite3_stmt* Statement, int32 Index, const FString& Value)
{
    FTCHARToUTF8 Bytes(*Value);
    return sqlite3_bind_text(Statement, Index, Bytes.Get(), Bytes.Length(), SQLITE_TRANSIENT) == SQLITE_OK;
}
FString EventPageColumnText(sqlite3_stmt* Statement, int32 Column)
{
    const auto* Bytes = sqlite3_column_text(Statement, Column);
    return Bytes ? FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Bytes))) : FString();
}
}

bool FOGSQLiteWorldStore::ListWorldEventsPage(const FOGEntityId& Entity, FName Type,
    bool bChronicleOnly, int32 Limit, const FOGWorldEventCursor& Before,
    TArray<FOGWorldEvent>& Out, FOGWorldEventCursor& Next, FString& Error) const
{
    Out.Reset(); Next = FOGWorldEventCursor(); Error.Reset();
    if (!Database || Limit <= 0 || Limit > 1000 || (Before.EventId.IsValid() && Before.WorldTick < 0))
    { Error = TEXT("Paged event history requires an open store, a valid cursor and a limit of 1..1000."); return false; }
    FString Sql = TEXT("SELECT event_id FROM world_events WHERE 1=1 ");
    if (Entity.IsValid()) Sql += TEXT("AND (primary_entity_id = ? OR related_entities_json LIKE ?) ");
    if (!Type.IsNone()) Sql += TEXT("AND event_type = ? ");
    if (bChronicleOnly) Sql += TEXT("AND chronicle_eligible = 1 ");
    if (Before.EventId.IsValid()) Sql += TEXT("AND (world_tick < ? OR (world_tick = ? AND event_id < ?)) ");
    Sql += TEXT("ORDER BY world_tick DESC, event_id DESC LIMIT ?;");
    sqlite3_stmt* Statement = nullptr;
    FTCHARToUTF8 Query(*Sql);
    if (sqlite3_prepare_v2(Database, Query.Get(), -1, &Statement, nullptr) != SQLITE_OK)
    { Error = LastError(TEXT("Prepare paged event history")); return false; }
    auto Fail = [&]() { Error = LastError(TEXT("Read paged event history")); sqlite3_finalize(Statement); Out.Reset(); return false; };
    int32 Index = 1;
    if (Entity.IsValid())
        if (!EventPageBindText(Statement, Index++, Entity.ToString()) ||
            !EventPageBindText(Statement, Index++, FString::Printf(TEXT("%%\"%s\"%%"), *Entity.ToString()))) return Fail();
    if (!Type.IsNone() && !EventPageBindText(Statement, Index++, Type.ToString())) return Fail();
    if (Before.EventId.IsValid())
    {
        if (sqlite3_bind_int64(Statement, Index++, Before.WorldTick) != SQLITE_OK ||
            sqlite3_bind_int64(Statement, Index++, Before.WorldTick) != SQLITE_OK ||
            !EventPageBindText(Statement, Index++, Before.EventId.ToString())) return Fail();
    }
    if (sqlite3_bind_int(Statement, Index, Limit) != SQLITE_OK) return Fail();
    TArray<FOGEntityId> Ids;
    for (;;)
    {
        const int32 Step = sqlite3_step(Statement);
        if (Step == SQLITE_DONE) break;
        if (Step != SQLITE_ROW) return Fail();
        FGuid Guid;
        if (!FGuid::Parse(EventPageColumnText(Statement, 0), Guid) || !Guid.IsValid())
        { sqlite3_finalize(Statement); Error = TEXT("Invalid event UUID in paged history."); return false; }
        Ids.Emplace(Guid);
    }
    sqlite3_finalize(Statement);
    for (const FOGEntityId& Id : Ids)
    {
        FOGWorldEvent Event; bool bFound = false;
        if (!TryReadWorldEvent(Id, bFound, Event, Error) || !bFound)
        { if (Error.IsEmpty()) Error = TEXT("A paged event disappeared from the append-only ledger."); Out.Reset(); return false; }
        Out.Add(MoveTemp(Event));
    }
    if (Out.Num() == Limit) { Next.WorldTick = Out.Last().WorldTick; Next.EventId = Out.Last().EventId; }
    return true;
}
