import { Trigger } from "@app/utils/rxjs/triggers";

export interface StateIPC {
  trigger: Trigger; // Renderer triggers like button clicks
  persisted: any;
}
