import { app } from "novadesk";

console.log("=== AppStorageAPI Integration ===");

function pass(name, detail) {
  console.log(`[PASS] ${name}${detail ? " -> " + detail : ""}`);
}

function fail(name, expected, actual) {
  console.log(`[FAIL] ${name} -> expected=${expected} actual=${actual}`);
}

function assertEq(name, actual, expected) {
  if (actual === expected) {
    pass(name, `expected=${expected} actual=${actual}`);
  } else {
    fail(name, expected, actual);
  }
}

const baseKey = "integration.app.storage";
const objKey = baseKey + ".object";
const strKey = baseKey + ".string";

app.storage.remove(objKey);
app.storage.remove(strKey);

const setStringOk = app.storage.set(strKey, "hello-storage");
assertEq("set string", setStringOk, true);

const readString = app.storage.get(strKey);
assertEq("get string", readString, "hello-storage");

const setObjOk = app.storage.set(objKey, {
  enabled: true,
  count: 3,
  tags: ["a", "b"],
  nested: { value: "ok" }
});
assertEq("set object", setObjOk, true);

const readObj = app.storage.get(objKey, null);
if (readObj && readObj.enabled === true && readObj.count === 3 && readObj.nested && readObj.nested.value === "ok") {
  pass("get object", JSON.stringify(readObj));
} else {
  fail("get object", '{"enabled":true,"count":3,...}', JSON.stringify(readObj));
}

const fallback = app.storage.get(baseKey + ".missing", 42);
assertEq("get fallback", fallback, 42);

const removed = app.storage.remove(strKey);
assertEq("remove existing", removed, true);

const afterRemove = app.storage.get(strKey, "missing");
assertEq("get after remove", afterRemove, "missing");

console.log("[PASS] AppStorageAPI completed");
