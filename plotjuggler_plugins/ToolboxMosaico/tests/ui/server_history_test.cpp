#include "ui/server_history.h"

#include <gtest/gtest.h>

#include <QString>
#include <QStringList>

// ---------- normalizeServerKey ----------

TEST(ServerHistoryNormalize, StripsGrpcScheme)
{
  EXPECT_EQ(normalizeServerKey("grpc://demo.mosaico.dev:6726"),
            QString("demo.mosaico.dev:6726"));
}

TEST(ServerHistoryNormalize, StripsGrpcTlsScheme)
{
  EXPECT_EQ(normalizeServerKey("grpc+tls://demo.mosaico.dev:6726"),
            QString("demo.mosaico.dev:6726"));
}

TEST(ServerHistoryNormalize, StripsTrailingSlash)
{
  EXPECT_EQ(normalizeServerKey("grpc://demo.mosaico.dev:6726/"),
            QString("demo.mosaico.dev:6726"));
}

TEST(ServerHistoryNormalize, LowercasesHost)
{
  EXPECT_EQ(normalizeServerKey("grpc://DEMO.Mosaico.DEV:6726"),
            QString("demo.mosaico.dev:6726"));
}

TEST(ServerHistoryNormalize, PreservesPortCase)
{
  EXPECT_EQ(normalizeServerKey("demo.mosaico.dev:6726"),
            QString("demo.mosaico.dev:6726"));
}

TEST(ServerHistoryNormalize, BareHostWithoutPortIsLowercased)
{
  EXPECT_EQ(normalizeServerKey("EXAMPLE.Host"),
            QString("example.host"));
}

TEST(ServerHistoryNormalize, TrimsWhitespace)
{
  EXPECT_EQ(normalizeServerKey("  grpc://demo.mosaico.dev:6726  "),
            QString("demo.mosaico.dev:6726"));
}

TEST(ServerHistoryNormalize, EmptyInputYieldsEmpty)
{
  EXPECT_EQ(normalizeServerKey(""), QString(""));
  EXPECT_EQ(normalizeServerKey("   "), QString(""));
}

TEST(ServerHistoryNormalize, MalformedSchemeOnlyYieldsEmpty)
{
  EXPECT_EQ(normalizeServerKey("grpc://"), QString(""));
}

// ---------- promoteToHead ----------

TEST(ServerHistoryPromote, PrependsNewKey)
{
  QStringList h = {"a:1", "b:2"};
  EXPECT_EQ(promoteToHead(h, "c:3", 10),
            (QStringList{"c:3", "a:1", "b:2"}));
}

TEST(ServerHistoryPromote, MovesExistingKeyToFront)
{
  QStringList h = {"a:1", "b:2", "c:3"};
  EXPECT_EQ(promoteToHead(h, "c:3", 10),
            (QStringList{"c:3", "a:1", "b:2"}));
}

TEST(ServerHistoryPromote, NoDuplicatesAfterPromotion)
{
  QStringList h = {"a:1", "b:2"};
  EXPECT_EQ(promoteToHead(h, "a:1", 10),
            (QStringList{"a:1", "b:2"}));
}

TEST(ServerHistoryPromote, CapEvictsOldest)
{
  QStringList h = {"a:1", "b:2", "c:3"};
  EXPECT_EQ(promoteToHead(h, "d:4", 3),
            (QStringList{"d:4", "a:1", "b:2"}));
}

TEST(ServerHistoryPromote, CapOfZeroYieldsEmpty)
{
  QStringList h = {"a:1"};
  EXPECT_EQ(promoteToHead(h, "b:2", 0), QStringList{});
}

TEST(ServerHistoryPromote, EmptyKeyIsRejected)
{
  QStringList h = {"a:1"};
  EXPECT_EQ(promoteToHead(h, "", 10), (QStringList{"a:1"}));
}
