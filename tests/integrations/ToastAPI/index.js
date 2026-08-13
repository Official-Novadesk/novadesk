import { toast } from "novadesk";

console.log("=== ToastAPI ===");
console.log("compatible:", toast.isCompatible());

const initialized = toast.initialize({
  appName: "Novadesk",
  companyName: "OfficialNovadesk",
  productName: "Novadesk"
});

console.log("initialized:", initialized);

// const id = toast.show({
//   title: "Novadesk Toast",
//   message: "Toast API is available from the novadesk module.",
//   duration: "short",
//   actions: ["Open", "Dismiss"],
//   onActivated: (event) => {
//     console.log("toast activated:", event.toastId);
//   },
//   onAction: (event) => {
//     if (event.actionIndex === 0) {
//       console.log("toast action: open");
//     } else if (event.actionIndex === 1) {
//       console.log("toast action: dismiss");
//     }
//     console.log("toast action:", event.toastId, event.actionIndex);
//   },
//   onDismissed: (event) => {
//     console.log("toast dismissed:", event.toastId, event.reason);
//   },
//   onFailed: (event) => {
//     console.log("toast failed:", event.toastId);
//   }
// });

// console.log("toast id:", id);
// if (id === null) {
//   console.log("toast error:", toast.getLastError());
// }

const inputId = toast.show({
  title: "Novadesk Toast Input",
  message: "Type a reply and press Send.",
  duration: "long",
  // input: true,
  scenario: "incomingCall",
  audio:"call",
  actions: ["Send"],
  onInput: (event) => {
    console.log("toast input:", event.toastId, event.input);
  },
  onAction: (event) => {
    console.log("toast input action:", event.toastId, event.actionIndex);
  },
  onDismissed: (event) => {
    console.log("toast input dismissed:", event.toastId, event.reason);
  },
  onFailed: (event) => {
    console.log("toast input failed:", event.toastId);
  }
});

console.log("toast input id:", inputId);
if (inputId === null) {
  console.log("toast input error:", toast.getLastError());
}
